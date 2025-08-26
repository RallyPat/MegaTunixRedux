import 'package:flutter/material.dart';
import '../views/dashboard_view.dart';
import '../views/tables_view.dart';

/// Modern navigation router for MegaTunix Redux
/// Supports named routes and programmatic navigation
class AppRouter {
  static const String dashboard = '/dashboard';
  static const String tables = '/tables';
  static const String tools = '/tools';
  static const String settings = '/settings';
  static const String diagnostics = '/diagnostics';

  static Route<dynamic> generateRoute(RouteSettings settings) {
    final routeName = settings.name;
    
    if (routeName == dashboard) {
      return _buildRoute(
        const DashboardView(),
        settings: settings,
      );
    } else if (routeName == tables) {
      return _buildRoute(
        const TablesView(),
        settings: settings,
      );
    } else if (routeName == tools) {
      return _buildRoute(
        const ToolsView(),
        settings: settings,
      );
    } else if (routeName == AppRouter.settings) {
      return _buildRoute(
        const SettingsView(),
        settings: settings,
      );
    } else if (routeName == diagnostics) {
      return _buildRoute(
        const DiagnosticsView(),
        settings: settings,
      );
    } else {
      return _buildRoute(
        const DashboardView(),
        settings: settings,
      );
    }
  }

  static PageRoute<dynamic> _buildRoute(
    Widget child, {
    required RouteSettings settings,
  }) {
    return PageRouteBuilder<dynamic>(
      settings: settings,
      pageBuilder: (context, animation, secondaryAnimation) => child,
      transitionsBuilder: (context, animation, secondaryAnimation, child) {
        return FadeTransition(opacity: animation, child: child);
      },
      transitionDuration: const Duration(milliseconds: 200),
    );
  }
}

// Placeholder views - will be implemented next
class ToolsView extends StatelessWidget {
  const ToolsView({super.key});

  @override
  Widget build(BuildContext context) {
    return const Scaffold(
      body: Center(child: Text('Tools View')),
    );
  }
}

class SettingsView extends StatelessWidget {
  const SettingsView({super.key});

  @override
  Widget build(BuildContext context) {
    return const Scaffold(
      body: Center(child: Text('Settings View')),
    );
  }
}

class DiagnosticsView extends StatelessWidget {
  const DiagnosticsView({super.key});

  @override
  Widget build(BuildContext context) {
    return const Scaffold(
      body: Center(child: Text('Diagnostics View')),
    );
  }
}