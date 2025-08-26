import 'package:flutter/material.dart';

class TablesTab extends StatefulWidget {
  const TablesTab({super.key});

  @override
  State<TablesTab> createState() => _TablesTabState();
}

class _TablesTabState extends State<TablesTab> with SingleTickerProviderStateMixin {
  late TabController _tableTabController;

  @override
  void initState() {
    super.initState();
    _tableTabController = TabController(length: 3, vsync: this);
  }

  @override
  void dispose() {
    _tableTabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Container(
          color: Theme.of(context).colorScheme.surface,
          child: TabBar(
            controller: _tableTabController,
            tabs: const [
              Tab(text: 'VE Table'),
              Tab(text: 'Ignition Table'),
              Tab(text: 'AFR Table'),
            ],
          ),
        ),
        Expanded(
          child: TabBarView(
            controller: _tableTabController,
            children: [
              _buildTableEditor('VE', 'Volumetric Efficiency', '%'),
              _buildTableEditor('Ignition', 'Ignition Timing', '°BTDC'),
              _buildTableEditor('AFR', 'Air/Fuel Ratio', ':1'),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildTableEditor(String name, String fullName, String unit) {
    return Padding(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                '$fullName ($unit)',
                style: Theme.of(context).textTheme.headlineSmall,
              ),
              Row(
                children: [
                  ElevatedButton.icon(
                    onPressed: () {
                      // TODO: Load table
                    },
                    icon: const Icon(Icons.folder_open),
                    label: const Text('Load'),
                  ),
                  const SizedBox(width: 8),
                  ElevatedButton.icon(
                    onPressed: () {
                      // TODO: Save table
                    },
                    icon: const Icon(Icons.save),
                    label: const Text('Save'),
                  ),
                  const SizedBox(width: 8),
                  ElevatedButton.icon(
                    onPressed: () {
                      // TODO: Send to ECU
                    },
                    icon: const Icon(Icons.upload),
                    label: const Text('Send to ECU'),
                  ),
                ],
              ),
            ],
          ),
          
          const SizedBox(height: 16),
          
          // Table editor placeholder
          Expanded(
            child: Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  children: [
                    // Axis labels
                    Row(
                      children: [
                        const SizedBox(width: 40), // Corner space
                        Expanded(
                          child: Center(
                            child: Text(
                              'MAP (kPa)',
                              style: Theme.of(context).textTheme.titleSmall,
                            ),
                          ),
                        ),
                      ],
                    ),
                    
                    const SizedBox(height: 8),
                    
                    // Table grid
                    Expanded(
                      child: Row(
                        children: [
                          // RPM axis (vertical)
                          Column(
                            children: [
                              const SizedBox(height: 24),
                              Expanded(
                                child: RotatedBox(
                                  quarterTurns: 3,
                                  child: Center(
                                    child: Text(
                                      'RPM',
                                      style: Theme.of(context).textTheme.titleSmall,
                                    ),
                                  ),
                                ),
                              ),
                            ],
                          ),
                          
                          const SizedBox(width: 8),
                          
                          // Table grid
                          Expanded(
                            child: _buildTableGrid(name, unit),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTableGrid(String tableType, String unit) {
    // Sample data for demonstration
    final sampleData = List.generate(8, (row) {
      return List.generate(8, (col) {
        switch (tableType) {
          case 'VE':
            return (80 + (row * col * 2)).toDouble();
          case 'Ignition':
            return (10 + (row + col)).toDouble();
          case 'AFR':
            return (13.0 + (row * 0.2) + (col * 0.1));
          default:
            return 0.0;
        }
      });
    });

    return Container(
      decoration: BoxDecoration(
        border: Border.all(color: Colors.grey.shade300),
        borderRadius: BorderRadius.circular(4),
      ),
      child: Column(
        children: [
          // Column headers (MAP values)
          Container(
            height: 32,
            decoration: BoxDecoration(
              color: Theme.of(context).colorScheme.surfaceContainerHighest,
              borderRadius: const BorderRadius.vertical(top: Radius.circular(4)),
            ),
            child: Row(
              children: [
                // Corner cell
                Container(
                  width: 60,
                  alignment: Alignment.center,
                  child: Text(
                    'RPM\\MAP',
                    style: Theme.of(context).textTheme.bodySmall,
                  ),
                ),
                // MAP values
                for (int col = 0; col < 8; col++)
                  Expanded(
                    child: Container(
                      alignment: Alignment.center,
                      child: Text(
                        '${20 + (col * 30)}',
                        style: Theme.of(context).textTheme.bodySmall,
                      ),
                    ),
                  ),
              ],
            ),
          ),
          
          // Data rows
          Expanded(
            child: Column(
              children: [
                for (int row = 0; row < 8; row++)
                  Expanded(
                    child: Row(
                      children: [
                        // RPM value
                        Container(
                          width: 60,
                          alignment: Alignment.center,
                          decoration: BoxDecoration(
                            color: Theme.of(context).colorScheme.surfaceContainerHighest,
                            border: Border(
                              right: BorderSide(color: Colors.grey.shade300),
                              bottom: row < 7 ? BorderSide(color: Colors.grey.shade300) : BorderSide.none,
                            ),
                          ),
                          child: Text(
                            '${1000 + (row * 1000)}',
                            style: Theme.of(context).textTheme.bodySmall,
                          ),
                        ),
                        // Data cells
                        for (int col = 0; col < 8; col++)
                          Expanded(
                            child: Container(
                              alignment: Alignment.center,
                              decoration: BoxDecoration(
                                border: Border(
                                  right: col < 7 ? BorderSide(color: Colors.grey.shade300) : BorderSide.none,
                                  bottom: row < 7 ? BorderSide(color: Colors.grey.shade300) : BorderSide.none,
                                ),
                              ),
                              child: InkWell(
                                onTap: () {
                                  // TODO: Edit cell value
                                  _editCellValue(context, row, col, sampleData[row][col], unit);
                                },
                                child: Container(
                                  width: double.infinity,
                                  height: double.infinity,
                                  alignment: Alignment.center,
                                  child: Text(
                                    unit == ':1' 
                                        ? sampleData[row][col].toStringAsFixed(1)
                                        : sampleData[row][col].toStringAsFixed(0),
                                    style: Theme.of(context).textTheme.bodySmall,
                                  ),
                                ),
                              ),
                            ),
                          ),
                      ],
                    ),
                  ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  void _editCellValue(BuildContext context, int row, int col, double currentValue, String unit) {
    final controller = TextEditingController(text: currentValue.toString());
    
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Edit Cell ($row, $col)'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: controller,
              keyboardType: TextInputType.number,
              decoration: InputDecoration(
                labelText: 'Value ($unit)',
                border: const OutlineInputBorder(),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              // TODO: Update cell value
              Navigator.pop(context);
            },
            child: const Text('Update'),
          ),
        ],
      ),
    );
  }
}