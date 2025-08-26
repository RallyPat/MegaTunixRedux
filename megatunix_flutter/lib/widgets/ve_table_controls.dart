import 'package:flutter/material.dart';
import '../theme/ecu_theme.dart';

/// VE Table Controls Widget
/// Provides professional table editing operations and controls
class VETableControls extends StatelessWidget {
  final VoidCallback onInterpolate;
  final VoidCallback onSmooth;
  final VoidCallback onBackup;
  final VoidCallback onRestore;
  final VoidCallback onExport;
  final VoidCallback onImport;

  const VETableControls({
    super.key,
    required this.onInterpolate,
    required this.onSmooth,
    required this.onBackup,
    required this.onRestore,
    required this.onExport,
    required this.onImport,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        border: Border(
          bottom: BorderSide(
            color: Theme.of(context).colorScheme.outline.withOpacity(0.2),
            width: 1,
          ),
        ),
      ),
      child: SingleChildScrollView(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Section title
            Text(
              'Table Operations',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.w600,
                color: ECUTheme.getAccentColor('primary'),
              ),
            ),
            const SizedBox(height: 6),
            
            // Control buttons in rows
            Row(
              children: [
                Expanded(
                  child: _buildControlButton(
                    context,
                    icon: Icons.trending_up,
                    label: 'Interpolate',
                    description: 'Fill gaps between values',
                    onPressed: onInterpolate,
                    color: ECUTheme.getAccentColor('interpolate'),
                  ),
                ),
                const SizedBox(width: 6),
                Expanded(
                  child: _buildControlButton(
                    context,
                    icon: Icons.blur_on,
                    label: 'Smooth',
                    description: 'Reduce table roughness',
                    onPressed: onSmooth,
                    color: ECUTheme.getAccentColor('smooth'),
                  ),
                ),
                const SizedBox(width: 6),
                Expanded(
                  child: _buildControlButton(
                    context,
                    icon: Icons.backup,
                    label: 'Backup',
                    description: 'Save current table',
                    onPressed: onBackup,
                    color: ECUTheme.getAccentColor('backup'),
                  ),
                ),
              ],
            ),
            
            const SizedBox(height: 6),
            
            Row(
              children: [
                Expanded(
                  child: _buildControlButton(
                    context,
                    icon: Icons.restore,
                    label: 'Restore',
                    description: 'Load saved backup',
                    onPressed: onRestore,
                    color: ECUTheme.getAccentColor('restore'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: _buildControlButton(
                    context,
                    icon: Icons.upload_file,
                    label: 'Export',
                    description: 'Save to file',
                    onPressed: onExport,
                    color: ECUTheme.getAccentColor('export'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: _buildControlButton(
                    context,
                    icon: Icons.download,
                    label: 'Import',
                    description: 'Load from file',
                    onPressed: onImport,
                    color: ECUTheme.getAccentColor('import'),
                  ),
                ),
              ],
            ),
            
            const SizedBox(height: 16),
            
            // Collapsible Legend Panel
            ExpansionTile(
              title: Row(
                children: [
                  Icon(Icons.help_outline, color: ECUTheme.getAccentColor('info')),
                  const SizedBox(width: 8),
                  Text(
                    'Keyboard Shortcuts & Functions',
                    style: TextStyle(
                      fontSize: 14,
                      fontWeight: FontWeight.w600,
                      color: ECUTheme.getAccentColor('info'),
                    ),
                  ),
                ],
              ),
              children: [
                Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      _buildLegendSection(
                        'Table Navigation:',
                        [
                          'Arrow Keys: Move between cells',
                          'Tab/Shift+Tab: Move between cells',
                          'Home/End: Move to row start/end',
                          'Page Up/Down: Move between rows',
                        ],
                      ),
                      const SizedBox(height: 16),
                      _buildLegendSection(
                        'Cell Selection:',
                        [
                          'Ctrl+Click: Select individual cells',
                          'Shift+Click: Select range of cells',
                          'Ctrl+A: Select all cells',
                          'Ctrl+Shift+A: Deselect all cells',
                        ],
                      ),
                      const SizedBox(height: 16),
                      _buildLegendSection(
                        'Table Operations:',
                        [
                          'Interpolate: Fill gaps between values',
                          'Smooth: Reduce table roughness',
                          'Backup: Save current table state',
                          'Restore: Load saved backup',
                          'Export: Save table to file (CSV, BIN, etc.)',
                          'Import: Load table from file',
                        ],
                      ),
                    ],
                  ),
                ),
              ],
            ),
            
            const SizedBox(height: 16),
            
            // Quick info panel
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: ECUTheme.getAccentColor('info').withOpacity(0.1),
                borderRadius: BorderRadius.circular(8),
                border: Border.all(
                  color: ECUTheme.getAccentColor('info').withOpacity(0.3),
                  width: 1,
                ),
              ),
              child: Row(
                children: [
                  Icon(
                    Icons.info_outline,
                    color: ECUTheme.getAccentColor('info'),
                    size: 20,
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Text(
                      'Use Ctrl+Click to select multiple cells, Shift+Click for ranges. '
                      'Right-click for context menu with additional operations.',
                      style: TextStyle(
                        fontSize: 12,
                        color: ECUTheme.getAccentColor('info'),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
  
  /// Build individual control button
  Widget _buildControlButton(
    BuildContext context, {
    required IconData icon,
    required String label,
    required String description,
    required VoidCallback onPressed,
    required Color color,
  }) {
    return Container(
      height: 52, // Increased from 50 to 52 to prevent overflow
      child: ElevatedButton(
        onPressed: onPressed,
        style: ElevatedButton.styleFrom(
          backgroundColor: color.withOpacity(0.1),
          foregroundColor: color,
          elevation: 0,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
            side: BorderSide(
              color: color.withOpacity(0.3),
              width: 1,
            ),
          ),
          padding: const EdgeInsets.all(6), // Reduced from 8 to 6
        ),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          mainAxisSize: MainAxisSize.min, // Added to prevent expansion
          children: [
            Icon(icon, size: 18), // Reduced from 20 to 18
            const SizedBox(height: 1), // Reduced from 2 to 1
            Text(
              label,
              style: const TextStyle(
                fontSize: 10, // Reduced from 11 to 10
                fontWeight: FontWeight.w600,
              ),
              textAlign: TextAlign.center,
            ),
            Text(
              description,
              style: TextStyle(
                fontSize: 8, // Reduced from 9 to 8
                color: color.withOpacity(0.7),
              ),
              textAlign: TextAlign.center,
            ),
          ],
        ),
      ),
    );
  }
  
  /// Build legend section with title and bullet points
  Widget _buildLegendSection(String title, List<String> items) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          title,
          style: TextStyle(
            fontSize: 13,
            fontWeight: FontWeight.w700,
            color: ECUTheme.getAccentColor('primary'),
          ),
        ),
        const SizedBox(height: 6),
        ...items.map((item) => Padding(
          padding: const EdgeInsets.only(left: 16, bottom: 2),
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                '• ',
                style: TextStyle(
                  color: ECUTheme.getAccentColor('info'),
                  fontWeight: FontWeight.bold,
                ),
              ),
              Expanded(
                child: Text(
                  item,
                  style: TextStyle(
                    fontSize: 11,
                    color: ECUTheme.getAccentColor('info'),
                  ),
                ),
              ),
            ],
          ),
        )),
      ],
    );
  }
}
