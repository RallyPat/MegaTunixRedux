import 'package:flutter/material.dart';
import '../widgets/sidebar_navigation.dart';
import '../core/app_router.dart';

/// Modern adaptive layout shell for MegaTunix Redux
/// Provides responsive sidebar navigation and content area
class AdaptiveLayoutShell extends StatefulWidget {
  const AdaptiveLayoutShell({super.key});

  @override
  State<AdaptiveLayoutShell> createState() => _AdaptiveLayoutShellState();
}

class _AdaptiveLayoutShellState extends State<AdaptiveLayoutShell> {
  int _selectedIndex = 0;
  final GlobalKey<NavigatorState> _navigatorKey = GlobalKey<NavigatorState>();

  static const List<String> _routes = [
    AppRouter.dashboard,
    AppRouter.tables,
    AppRouter.tools,
    AppRouter.diagnostics,
    AppRouter.settings,
  ];

  @override
  void initState() {
    super.initState();
    // Navigate to initial route
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _navigateToRoute(_routes[_selectedIndex]);
    });
  }

  void _onNavigationItemTapped(int index) {
    if (index != _selectedIndex) {
      setState(() {
        _selectedIndex = index;
      });
      _navigateToRoute(_routes[index]);
    }
  }

  void _navigateToRoute(String routeName) {
    _navigatorKey.currentState?.pushReplacementNamed(routeName);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: LayoutBuilder(
        builder: (context, constraints) {
          final isWideScreen = constraints.maxWidth >= 1200;
          
          if (isWideScreen) {
            // Desktop layout with sidebar
            return Row(
              children: [
                SidebarNavigation(
                  selectedIndex: _selectedIndex,
                  onItemTapped: _onNavigationItemTapped,
                ),
                Expanded(
                  child: _buildContentArea(),
                ),
              ],
            );
          } else {
            // Mobile/tablet layout with drawer or bottom nav
            return Scaffold(
              drawer: _buildDrawer(),
              appBar: _buildAppBar(),
              body: _buildContentArea(),
            );
          }
        },
      ),
    );
  }

  Widget _buildContentArea() {
    return Navigator(
      key: _navigatorKey,
      onGenerateRoute: AppRouter.generateRoute,
      initialRoute: _routes[_selectedIndex],
    );
  }

  PreferredSizeWidget _buildAppBar() {
    final theme = Theme.of(context);
    
    return AppBar(
      title: Row(
        children: [
          Icon(Icons.speed, color: theme.colorScheme.primary),
          const SizedBox(width: 8),
          const Text('MegaTunix Redux'),
        ],
      ),
      backgroundColor: theme.colorScheme.surface,
      elevation: 0,
      scrolledUnderElevation: 1,
      actions: [
        _buildConnectionIndicator(),
        const SizedBox(width: 16),
      ],
    );
  }

  Widget _buildDrawer() {
    return Drawer(
      child: Column(
        children: [
          const DrawerHeader(
            decoration: BoxDecoration(
              gradient: LinearGradient(
                begin: Alignment.topLeft,
                end: Alignment.bottomRight,
                colors: [
                  Color(0xFF42CEDB),
                  Color(0xFF3BB8C4),
                ],
              ),
            ),
            child: Row(
              children: [
                Icon(
                  Icons.speed,
                  color: Colors.white,
                  size: 32,
                ),
                SizedBox(width: 12),
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      'MegaTunix',
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    Text(
                      'Redux',
                      style: TextStyle(
                        color: Colors.white70,
                        fontSize: 14,
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
          Expanded(
            child: ListView(
              children: [
                _buildDrawerItem(0, Icons.dashboard, 'Dashboard'),
                _buildDrawerItem(1, Icons.table_chart, 'Tables'),
                _buildDrawerItem(2, Icons.build, 'Tools'),
                _buildDrawerItem(3, Icons.error, 'Diagnostics'),
                _buildDrawerItem(4, Icons.settings, 'Settings'),
              ],
            ),
          ),
          const Divider(height: 1),
          Padding(
            padding: const EdgeInsets.all(16),
            child: Text(
              'v2.0.0-dev',
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                color: Theme.of(context).colorScheme.onSurfaceVariant,
              ),
              textAlign: TextAlign.center,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildDrawerItem(int index, IconData icon, String title) {
    final isSelected = _selectedIndex == index;
    
    return ListTile(
      leading: Icon(icon),
      title: Text(title),
      selected: isSelected,
      onTap: () {
        Navigator.of(context).pop(); // Close drawer
        _onNavigationItemTapped(index);
      },
    );
  }

  Widget _buildConnectionIndicator() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      decoration: BoxDecoration(
        color: Colors.red.withOpacity(0.1),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.red.withOpacity(0.3)),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 6,
            height: 6,
            decoration: const BoxDecoration(
              color: Colors.red,
              shape: BoxShape.circle,
            ),
          ),
          const SizedBox(width: 6),
          Text(
            'Disconnected',
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
              color: Colors.red,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }
}