/// ECU Protocol Factory
/// Creates appropriate protocol handlers for different ECU types
/// Supports Speeduino, EpicECU, MegaSquirt, and extensible for others

import '../models/ecu_data.dart';
import 'ecu_protocol_handler.dart';
import 'speeduino_protocol_handler.dart';
import 'epic_ecu_protocol_handler.dart';
import 'megasquirt_protocol_handler.dart';

/// Factory for creating ECU protocol handlers
class ECUProtocolFactory {
  /// Create protocol handler based on ECU type
  static ECUProtocolHandler createHandler(ECUProtocol protocol) {
    switch (protocol) {
      case ECUProtocol.speeduino:
        return SpeeduinoProtocolHandler();
      case ECUProtocol.epicEFI:
        return EpicECUProtocolHandler();
      case ECUProtocol.megasquirt:
        return MegaSquirtProtocolHandler();
      default:
        throw ArgumentError('Unsupported ECU protocol: $protocol');
    }
  }
  
  /// Get supported protocols
  static List<ECUProtocol> get supportedProtocols => [
    ECUProtocol.speeduino,
    ECUProtocol.epicEFI,
    ECUProtocol.megasquirt,
  ];
  
  /// Get protocol display names
  static Map<ECUProtocol, String> get protocolNames => {
    ECUProtocol.speeduino: 'Speeduino',
    ECUProtocol.epicEFI: 'EpicECU',
    ECUProtocol.megasquirt: 'MegaSquirt',
  };
}
