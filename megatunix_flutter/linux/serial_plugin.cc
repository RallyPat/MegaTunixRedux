#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

// Serial plugin for Linux
class SerialPlugin {
 public:
  SerialPlugin(FlMethodChannel* channel) : method_channel_(channel) {
    // Set up method call handler
    g_autoptr(FlMethodCall) call = nullptr;
    fl_method_channel_set_method_call_handler(
        method_channel_,
        [](FlMethodChannel* channel, FlMethodCall* method_call, gpointer user_data) {
          static_cast<SerialPlugin*>(user_data)->HandleMethodCall(method_call);
        },
        this, nullptr);
  }

  ~SerialPlugin() {
    ClosePort();
  }

 private:
  FlMethodChannel* method_channel_;
  int serial_fd_ = -1;
  GThread* read_thread_ = nullptr;
  gboolean stop_reading_ = FALSE;

  void HandleMethodCall(FlMethodCall* method_call) {
    g_autoptr(FlMethodResponse) response = nullptr;

    const gchar* method = fl_method_call_get_name(method_call);
    FlValue* args = fl_method_call_get_args(method_call);

    if (strcmp(method, "connect") == 0) {
      response = Connect(args);
    } else if (strcmp(method, "disconnect") == 0) {
      response = Disconnect();
    } else if (strcmp(method, "sendData") == 0) {
      response = SendData(args);
    } else if (strcmp(method, "getAvailablePorts") == 0) {
      response = GetAvailablePorts();
    } else if (strcmp(method, "isPortAvailable") == 0) {
      response = IsPortAvailable(args);
    } else {
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "UNKNOWN_METHOD", "Unknown method", nullptr));
    }

    fl_method_call_respond(method_call, response, nullptr);
  }

  FlMethodResponse* Connect(FlValue* args) {
    printf("SerialPlugin: Connect method called\n");

    if (serial_fd_ != -1) {
      printf("SerialPlugin: Already connected to serial port\n");
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "ALREADY_CONNECTED", "Already connected to a serial port", nullptr));
    }

    if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
      printf("SerialPlugin: Invalid arguments - not a map\n");
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "INVALID_ARGS", "Invalid arguments", nullptr));
    }

    const gchar* port = nullptr;
    gint baud_rate = 115200;

    // Extract port and baud rate from arguments
    for (size_t i = 0; i < fl_value_get_length(args); i++) {
      FlValue* key = fl_value_get_map_key(args, i);
      FlValue* value = fl_value_get_map_value(args, i);

      if (strcmp(fl_value_get_string(key), "port") == 0) {
        port = fl_value_get_string(value);
      } else if (strcmp(fl_value_get_string(key), "baudRate") == 0) {
        baud_rate = fl_value_get_int(value);
      }
    }

    if (!port) {
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "INVALID_PORT", "Port not specified", nullptr));
    }

    // Open serial port
    serial_fd_ = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd_ == -1) {
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "OPEN_FAILED", strerror(errno), nullptr));
    }

    // Configure serial port
    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(serial_fd_, &tty) != 0) {
      close(serial_fd_);
      serial_fd_ = -1;
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "CONFIG_FAILED", "Failed to get terminal attributes", nullptr));
    }

    // Set baud rate
    speed_t baud_speed;
    switch (baud_rate) {
      case 9600: baud_speed = B9600; break;
      case 19200: baud_speed = B19200; break;
      case 38400: baud_speed = B38400; break;
      case 57600: baud_speed = B57600; break;
      case 115200: baud_speed = B115200; break;
      case 230400: baud_speed = B230400; break;
      case 460800: baud_speed = B460800; break;
      default:
        baud_speed = B115200;
        break;
    }

    cfsetospeed(&tty, baud_speed);
    cfsetispeed(&tty, baud_speed);

    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB;  // No parity
    tty.c_cflag &= ~CSTOPB;  // 1 stop bit
    tty.c_cflag &= ~CSIZE;   // Clear data size bits
    tty.c_cflag |= CS8;      // 8 data bits
    tty.c_cflag |= CREAD;    // Enable receiver
    tty.c_cflag |= CLOCAL;   // Ignore modem control lines

    // Disable canonical mode, echo, signals
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ISIG;

    // Disable input/output processing
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;

    // Set timeouts
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;  // 1 second timeout

    if (tcsetattr(serial_fd_, TCSANOW, &tty) != 0) {
      close(serial_fd_);
      serial_fd_ = -1;
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "CONFIG_FAILED", "Failed to set terminal attributes", nullptr));
    }

    // Start read thread
    stop_reading_ = FALSE;
    read_thread_ = g_thread_new("serial_read", ReadThreadFunc, this);

    return FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(TRUE)));
  }

  FlMethodResponse* Disconnect() {
    ClosePort();
    return FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
  }

  FlMethodResponse* SendData(FlValue* args) {
    if (serial_fd_ == -1) {
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "NOT_CONNECTED", "Not connected to serial port", nullptr));
    }

    if (fl_value_get_type(args) != FL_VALUE_TYPE_UINT8_LIST) {
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "INVALID_DATA", "Data must be Uint8List", nullptr));
    }

    size_t length = fl_value_get_length(args);
    const uint8_t* data = fl_value_get_uint8_list(args);

    ssize_t written = write(serial_fd_, data, length);
    if (written == -1) {
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "WRITE_FAILED", strerror(errno), nullptr));
    }

    return FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(TRUE)));
  }

  FlMethodResponse* GetAvailablePorts() {
    g_autoptr(FlValue) ports = fl_value_new_list();

    // Common serial port patterns
    const char* port_patterns[] = {
      "/dev/ttyUSB*",
      "/dev/ttyACM*",
      "/dev/ttyS*",
      nullptr
    };

    for (const char** pattern = port_patterns; *pattern != nullptr; ++pattern) {
      // Use glob to find matching ports
      glob_t glob_result;
      if (glob(*pattern, GLOB_NOSORT, nullptr, &glob_result) == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
          // Check if port is accessible
          if (access(glob_result.gl_pathv[i], R_OK | W_OK) == 0) {
            fl_value_append(ports, fl_value_new_string(glob_result.gl_pathv[i]));
          }
        }
        globfree(&glob_result);
      }
    }

    return FL_METHOD_RESPONSE(fl_method_success_response_new(ports));
  }

  FlMethodResponse* IsPortAvailable(FlValue* args) {
    if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "INVALID_ARGS", "Invalid arguments", nullptr));
    }

    const gchar* port = nullptr;
    for (size_t i = 0; i < fl_value_get_length(args); i++) {
      FlValue* key = fl_value_get_map_key(args, i);
      FlValue* value = fl_value_get_map_value(args, i);

      if (strcmp(fl_value_get_string(key), "port") == 0) {
        port = fl_value_get_string(value);
        break;
      }
    }

    if (!port) {
      return FL_METHOD_RESPONSE(fl_method_error_response_new(
          "INVALID_PORT", "Port not specified", nullptr));
    }

    // Check if port exists and is accessible
    gboolean available = (access(port, R_OK | W_OK) == 0);

    return FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(available)));
  }

  void ClosePort() {
    if (read_thread_) {
      stop_reading_ = TRUE;
      g_thread_join(read_thread_);
      read_thread_ = nullptr;
    }

    if (serial_fd_ != -1) {
      close(serial_fd_);
      serial_fd_ = -1;
    }
  }

  static gpointer ReadThreadFunc(gpointer data) {
    SerialPlugin* plugin = static_cast<SerialPlugin*>(data);

    while (!plugin->stop_reading_) {
      if (plugin->serial_fd_ != -1) {
        uint8_t buffer[256];
        ssize_t bytes_read = read(plugin->serial_fd_, buffer, sizeof(buffer));

        if (bytes_read > 0) {
          // Send data to Flutter
          g_autoptr(FlValue) data_value = fl_value_new_uint8_list(buffer, bytes_read);
          fl_method_channel_invoke_method(
              plugin->method_channel_,
              "onDataReceived",
              data_value,
              nullptr, nullptr, nullptr);
        } else if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
          // Send error to Flutter
          g_autoptr(FlValue) error_value = fl_value_new_string(strerror(errno));
          fl_method_channel_invoke_method(
              plugin->method_channel_,
              "onError",
              error_value,
              nullptr, nullptr, nullptr);
          break;
        }

        // Small delay to prevent busy waiting
        g_usleep(10000);  // 10ms
      } else {
        g_usleep(100000);  // 100ms
      }
    }

    return nullptr;
  }
};

// Plugin entry point
void serial_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  g_autoptr(FlMethodChannel) channel = fl_method_channel_new(
      fl_plugin_registrar_get_messenger(registrar),
      "com.megatunix.serial",
      FL_METHOD_CODEC(fl_standard_method_codec_new()));

  SerialPlugin* plugin = new SerialPlugin(channel);
}