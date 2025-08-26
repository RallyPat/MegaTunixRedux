import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../widgets/ecu_gauge_cluster.dart';
import '../widgets/ecu_status_panel.dart';
import '../widgets/ecu_connection_panel.dart';
import '../widgets/persistent_navigation_layout.dart';
import '../services/ecu_service.dart';
import '../models/ecu_data.dart';
import 've_table_editor_screen.dart';

/// Main ECU Dashboard Screen
/// Provides the primary interface for ECU tuning software
class ECUDashboardScreen extends StatefulWidget {
  const ECUDashboardScreen({super.key});

  @override
  State<ECUDashboardScreen> createState() => _ECUDashboardScreenState();
}

class _ECUDashboardScreenState extends State<ECUDashboardScreen>
    with TickerProviderStateMixin {
  late AnimationController _fadeController;
  late AnimationController _slideController;
  late Animation<double> _fadeAnimation;
  late Animation<Offset> _slideAnimation;
  
  // Navigation drawer state
  bool _isDrawerPinned = false;

  @override
  void initState() {
    super.initState();
    
    // Initialize animations for smooth entry
    _fadeController = AnimationController(
      duration: const Duration(milliseconds: 800),
      vsync: this,
    );
    
    _slideController = AnimationController(
      duration: const Duration(milliseconds: 600),
      vsync: this,
    );
    
    _fadeAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _fadeController,
      curve: Curves.easeOut,
    ));
    
    _slideAnimation = Tween<Offset>(
      begin: const Offset(0, 0.3),
      end: Offset.zero,
    ).animate(CurvedAnimation(
      parent: _slideController,
      curve: Curves.easeOutCubic,
    ));
    
    // Start animations
    _fadeController.forward();
    _slideController.forward();
  }
  
  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    
    // Start mock data generation after dependencies are available
    final ecuService = Provider.of<ECUService>(context, listen: false);
    if (ecuService.connectionState == ECUConnectionState.disconnected) {
      ecuService.startMockDataGeneration();
    }
  }

  @override
  void dispose() {
    _fadeController.dispose();
    _slideController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return PersistentNavigationLayout(
      title: 'MegaTunix Redux - ECU Tuning',
      showBackButton: false,
      actions: [
        // Theme toggle button
        IconButton(
          icon: Icon(
            Theme.of(context).brightness == Brightness.dark
                ? Icons.light_mode
                : Icons.dark_mode,
          ),
          onPressed: () {
            // TODO: Implement theme switching
          },
          tooltip: 'Toggle Theme',
        ),
        
        // Settings button
        IconButton(
          icon: const Icon(Icons.settings),
          onPressed: () {
            // TODO: Open settings
          },
          tooltip: 'Settings',
        ),
      ],
      floatingActionButton: FloatingActionButton.extended(
        onPressed: () {
          // TODO: Quick ECU connection
        },
        icon: const Icon(Icons.link),
        label: const Text('Connect ECU'),
        backgroundColor: Theme.of(context).colorScheme.primary,
        foregroundColor: Theme.of(context).colorScheme.onPrimary,
      ),
      child: FadeTransition(
        opacity: _fadeAnimation,
        child: SlideTransition(
          position: _slideAnimation,
          child: SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // ECU Connection Status
                ECUConnectionPanel(),
                
                SizedBox(height: 24),
                
                // ECU Status Overview
                ECUStatusPanel(),
                
                SizedBox(height: 24),
                
                // Real-time Gauge Cluster
                Consumer<ECUService>(
                  builder: (context, ecuService, child) {
                    // Get current ECU data or use default
                    final ecuData = ecuService.currentData ?? SpeeduinoData.defaultValues();
                    print('Dashboard received ECU data: RPM=${ecuData.rpm}, MAP=${ecuData.map}');
                    return Column(
                      children: [
                        // Debug button to start mock data - always show for testing
                        Container(
                          margin: const EdgeInsets.only(bottom: 16),
                          child: ElevatedButton.icon(
                            onPressed: () {
                              print('Starting mock data generation...');
                              ecuService.startMockDataGeneration();
                            },
                            icon: const Icon(Icons.play_arrow),
                            label: const Text('Start Demo Data'),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.teal,
                              foregroundColor: Colors.white,
                              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
                            ),
                          ),
                        ),
                        ECUGaugeCluster(
                          ecuData: ecuData,
                          showLabels: true,
                          showValues: true,
                          showUnits: true,
                          gaugeSize: 120.0, // Increased to match new default
                          enableAnimations: true,
                        ),
                      ],
                    );
                  },
                ),
                
                SizedBox(height: 24),
                
                // Quick Actions
                _QuickActionsSection(),
                
                SizedBox(height: 24),
                
                // Recent Activity
                _RecentActivitySection(),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

/// Quick Actions Section Widget
class _QuickActionsSection extends StatelessWidget {
  const _QuickActionsSection();

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Quick Actions',
              style: Theme.of(context).textTheme.headlineSmall,
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () {
                      // Navigate to VE table editor after current frame
                      WidgetsBinding.instance.addPostFrameCallback((_) {
                        Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (context) => const VETableEditorScreen(),
                          ),
                        );
                      });
                    },
                    icon: const Icon(Icons.table_chart),
                    label: const Text('VE Table Editor'),
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () {
                      // TODO: Open ignition table editor
                    },
                    icon: const Icon(Icons.timeline),
                    label: const Text('Ignition Table'),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () {
                      // TODO: Open data logging
                    },
                    icon: const Icon(Icons.analytics),
                    label: const Text('Data Logging'),
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () {
                      // TODO: Open diagnostics
                    },
                    icon: const Icon(Icons.bug_report),
                    label: const Text('Diagnostics'),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

/// Recent Activity Section Widget
class _RecentActivitySection extends StatelessWidget {
  const _RecentActivitySection();

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Recent Activity',
              style: Theme.of(context).textTheme.headlineSmall,
            ),
            const SizedBox(height: 16),
            _buildActivityItem(
              icon: Icons.link,
              title: 'ECU Connected',
              subtitle: 'Speeduino ECU detected on /dev/ttyACM0',
              time: '2 minutes ago',
              color: Colors.green,
            ),
            const Divider(),
            _buildActivityItem(
              icon: Icons.table_chart,
              title: 'VE Table Modified',
              subtitle: 'Cell (12, 8) updated from 95.2 to 97.8',
              time: '15 minutes ago',
              color: Colors.blue,
            ),
            const Divider(),
            _buildActivityItem(
              icon: Icons.save,
              title: 'Configuration Saved',
              subtitle: 'Current tune saved as "Street_Tune_v2"',
              time: '1 hour ago',
              color: Colors.orange,
            ),
          ],
        ),
      ),
    );
  }

  /// Build individual activity item
  Widget _buildActivityItem({
    required IconData icon,
    required String title,
    required String subtitle,
    required String time,
    required Color color,
  }) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Row(
        children: [
          Container(
            padding: const EdgeInsets.all(8),
            decoration: BoxDecoration(
              color: color.withOpacity(0.1),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Icon(
              icon,
              color: color,
              size: 20,
            ),
          ),
          const SizedBox(width: 16),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: const TextStyle(
                    fontSize: 16,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                Text(
                  subtitle,
                  style: TextStyle(
                    fontSize: 14,
                    color: Colors.grey[600],
                  ),
                ),
              ],
            ),
          ),
          Text(
            time,
            style: TextStyle(
              fontSize: 12,
              color: Colors.grey[500],
            ),
          ),
        ],
      ),
    );
  }
}
