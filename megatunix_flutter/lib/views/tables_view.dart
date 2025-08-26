import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:file_picker/file_picker.dart';
import '../services/ini_parser.dart';
import '../services/ini_msq_service.dart';
import '../services/realtime_data_service.dart';
import '../widgets/ini_table_view.dart';

/// Modern tables view for VE/Fuel/Timing tables
class TablesView extends StatefulWidget {
  const TablesView({super.key});

  @override
  State<TablesView> createState() => _TablesViewState();
}

class _TablesViewState extends State<TablesView> {
  int _selectedTableIndex = 0;
  RealtimeDataService? _realtimeService;

  @override
  void initState() {
    super.initState();
    _initRealtimeService();
  }

  void _initRealtimeService() {
    _realtimeService = RealtimeDataService();
    // Auto-start for development/demonstration
    _realtimeService?.startService();
  }

  @override
  void dispose() {
    _realtimeService?.stopService();
    _realtimeService?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Consumer2<INIECUManager, INIMSQFileService>(
      builder: (context, ecuManager, msqService, child) {
        // Check if ECU is loaded
        if (ecuManager.currentECU == null) {
          return _buildNoECUState(context, ecuManager);
        }

        // Check if MSQ file is loaded
        if (!msqService.hasFile) {
          return _buildNoFileState(context, msqService);
        }
        
        return Scaffold(
          body: Row(
            children: [
              _buildTableSelector(ecuManager.currentECU!),
              Expanded(
                child: _buildTableEditor(ecuManager.currentECU!, msqService),
              ),
            ],
          ),
          floatingActionButton: _buildRealtimeToggle(),
        );
      },
    );
  }

  Widget _buildNoECUState(BuildContext context, INIECUManager ecuManager) {
    final theme = Theme.of(context);
    
    return Center(
      child: Card(
        margin: const EdgeInsets.all(32),
        child: Padding(
          padding: const EdgeInsets.all(32),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(
                Icons.memory,
                size: 64,
                color: theme.colorScheme.primary,
              ),
              const SizedBox(height: 16),
              Text(
                'No ECU Definition Loaded',
                style: theme.textTheme.headlineSmall?.copyWith(
                  fontWeight: FontWeight.w600,
                ),
              ),
              const SizedBox(height: 8),
              Text(
                'Load a TunerStudio .ini file to define your ECU\'s tables and settings',
                style: theme.textTheme.bodyMedium?.copyWith(
                  color: theme.colorScheme.onSurfaceVariant,
                ),
                textAlign: TextAlign.center,
              ),
              const SizedBox(height: 24),
              FilledButton.icon(
                onPressed: () => _loadECUDefinition(ecuManager),
                icon: const Icon(Icons.upload_file),
                label: const Text('Load INI File'),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildNoFileState(BuildContext context, INIMSQFileService msqService) {
    final theme = Theme.of(context);
    
    return Center(
      child: Card(
        margin: const EdgeInsets.all(32),
        child: Padding(
          padding: const EdgeInsets.all(32),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(
                Icons.folder_open,
                size: 64,
                color: theme.colorScheme.primary,
              ),
              const SizedBox(height: 16),
              Text(
                'No Tune File Loaded',
                style: theme.textTheme.headlineSmall?.copyWith(
                  fontWeight: FontWeight.w600,
                ),
              ),
              const SizedBox(height: 8),
              Text(
                'Load an existing .msq file or create a new tune to start editing tables',
                style: theme.textTheme.bodyMedium?.copyWith(
                  color: theme.colorScheme.onSurfaceVariant,
                ),
                textAlign: TextAlign.center,
              ),
              const SizedBox(height: 24),
              Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  FilledButton.icon(
                    onPressed: () => _createNewFile(msqService),
                    icon: const Icon(Icons.add),
                    label: const Text('New Tune'),
                  ),
                  const SizedBox(width: 16),
                  OutlinedButton.icon(
                    onPressed: () => _loadFile(msqService),
                    icon: const Icon(Icons.folder_open),
                    label: const Text('Load File'),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildTableSelector(ECUDefinition ecuDefinition) {
    final theme = Theme.of(context);
    final tables = ecuDefinition.tables;
    
    return Container(
      width: 300,
      decoration: BoxDecoration(
        color: theme.colorScheme.surface,
        border: Border(
          right: BorderSide(
            color: theme.colorScheme.outlineVariant,
            width: 1,
          ),
        ),
      ),
      child: Column(
        children: [
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              border: Border(
                bottom: BorderSide(
                  color: theme.colorScheme.outlineVariant,
                  width: 1,
                ),
              ),
            ),
            child: Row(
              children: [
                Icon(
                  Icons.table_chart,
                  color: theme.colorScheme.primary,
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Tables',
                        style: theme.textTheme.titleMedium?.copyWith(
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                      Text(
                        '${tables.length} available',
                        style: theme.textTheme.bodySmall?.copyWith(
                          color: theme.colorScheme.onSurfaceVariant,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
          Expanded(
            child: ListView.builder(
              padding: const EdgeInsets.all(8),
              itemCount: tables.length,
              itemBuilder: (context, index) {
                return _buildTableListItem(index, tables[index]);
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTableListItem(int index, TableDefinition tableDef) {
    final theme = Theme.of(context);
    final isSelected = _selectedTableIndex == index;
    
    // Choose icon and color based on table type
    IconData icon = Icons.table_chart;
    Color color = theme.colorScheme.primary;
    
    if (tableDef.name.toLowerCase().contains('ve') || tableDef.name.toLowerCase().contains('fuel')) {
      icon = Icons.local_gas_station;
      color = const Color(0xFF4CAF50);
    } else if (tableDef.name.toLowerCase().contains('timing') || tableDef.name.toLowerCase().contains('spark')) {
      icon = Icons.access_time;
      color = const Color(0xFFFF6B35);
    } else if (tableDef.name.toLowerCase().contains('boost') || tableDef.name.toLowerCase().contains('air')) {
      icon = Icons.air;
      color = const Color(0xFFF44336);
    }
    
    return Card(
      margin: const EdgeInsets.symmetric(vertical: 4),
      elevation: 0,
      color: isSelected 
          ? color.withOpacity(0.1)
          : theme.colorScheme.surface,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(8),
        side: BorderSide(
          color: isSelected 
              ? color.withOpacity(0.3)
              : theme.colorScheme.outlineVariant,
          width: 1,
        ),
      ),
      child: InkWell(
        borderRadius: BorderRadius.circular(8),
        onTap: () {
          setState(() {
            _selectedTableIndex = index;
          });
        },
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  Container(
                    padding: const EdgeInsets.all(6),
                    decoration: BoxDecoration(
                      color: color.withOpacity(0.1),
                      borderRadius: BorderRadius.circular(6),
                    ),
                    child: Icon(
                      icon,
                      color: color,
                      size: 18,
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      tableDef.displayName,
                      style: theme.textTheme.titleSmall?.copyWith(
                        fontWeight: FontWeight.w500,
                        color: isSelected ? color : null,
                      ),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              Text(
                '${tableDef.rows}x${tableDef.cols} - ${tableDef.units}',
                style: theme.textTheme.bodySmall?.copyWith(
                  color: theme.colorScheme.onSurfaceVariant,
                ),
              ),
              if (tableDef.description.isNotEmpty)
                Text(
                  tableDef.description,
                  style: theme.textTheme.bodySmall?.copyWith(
                    color: theme.colorScheme.onSurfaceVariant,
                  ),
                  maxLines: 2,
                  overflow: TextOverflow.ellipsis,
                ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildTableEditor(ECUDefinition ecuDefinition, INIMSQFileService msqService) {
    if (ecuDefinition.tables.isEmpty) {
      return _buildNoTablesMessage();
    }

    final selectedTable = ecuDefinition.tables[_selectedTableIndex];
    final tableData = msqService.currentFile!.getTable(selectedTable.name);
    
    if (tableData == null) {
      return _buildTableNotFoundMessage(selectedTable.name);
    }

    final theme = Theme.of(context);
    
    return Column(
      children: [
        Container(
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            border: Border(
              bottom: BorderSide(
                color: theme.colorScheme.outlineVariant,
                width: 1,
              ),
            ),
          ),
          child: Row(
            children: [
              Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: theme.colorScheme.primary.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Icon(
                  Icons.table_chart,
                  color: theme.colorScheme.primary,
                  size: 20,
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      selectedTable.displayName,
                      style: theme.textTheme.titleMedium?.copyWith(
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                    Text(
                      '${selectedTable.rows}x${selectedTable.cols} ${selectedTable.units} - ${selectedTable.description}',
                      style: theme.textTheme.bodySmall?.copyWith(
                        color: theme.colorScheme.onSurfaceVariant,
                      ),
                    ),
                  ],
                ),
              ),
              if (msqService.hasUnsavedChanges)
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                  decoration: BoxDecoration(
                    color: Colors.orange.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(Icons.circle, color: Colors.orange, size: 8),
                      const SizedBox(width: 4),
                      Text(
                        'Unsaved',
                        style: theme.textTheme.bodySmall?.copyWith(
                          color: Colors.orange,
                        ),
                      ),
                    ],
                  ),
                ),
              const SizedBox(width: 8),
              FilledButton.icon(
                onPressed: msqService.hasUnsavedChanges ? () => _saveFile(msqService) : null,
                icon: const Icon(Icons.save, size: 18),
                label: const Text('Save'),
              ),
              const SizedBox(width: 8),
              OutlinedButton.icon(
                onPressed: () => _loadFile(msqService),
                icon: const Icon(Icons.folder_open, size: 18),
                label: const Text('Load'),
              ),
            ],
          ),
        ),
        Expanded(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: INITableView(
              tableDefinition: selectedTable,
              tableData: tableData,
              xAxis: msqService.currentFile!.getTableXAxis(selectedTable.name),
              yAxis: msqService.currentFile!.getTableYAxis(selectedTable.name),
              realtimeService: _realtimeService,
              showHeatmap: true,  // Explicitly enable heatmap
              showValues: true,   // Explicitly enable values
              onValueChanged: (row, col, value) {
                msqService.updateTableValue(selectedTable.name, row, col, value);
              },
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildNoTablesMessage() {
    final theme = Theme.of(context);
    
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.table_chart,
            size: 64,
            color: theme.colorScheme.onSurfaceVariant,
          ),
          const SizedBox(height: 16),
          Text(
            'No Tables Defined',
            style: theme.textTheme.headlineSmall?.copyWith(
              color: theme.colorScheme.onSurfaceVariant,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'This ECU definition contains no table definitions',
            style: theme.textTheme.bodyMedium?.copyWith(
              color: theme.colorScheme.onSurfaceVariant,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTableNotFoundMessage(String tableName) {
    final theme = Theme.of(context);
    
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.error_outline,
            size: 64,
            color: Colors.orange,
          ),
          const SizedBox(height: 16),
          Text(
            'Table Data Missing',
            style: theme.textTheme.headlineSmall?.copyWith(
              color: Colors.orange,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'Table "$tableName" is not available in the loaded tune file',
            style: theme.textTheme.bodyMedium?.copyWith(
              color: theme.colorScheme.onSurfaceVariant,
            ),
            textAlign: TextAlign.center,
          ),
        ],
      ),
    );
  }

  Widget _buildRealtimeToggle() {
    if (_realtimeService == null) return const SizedBox.shrink();
    
    return ChangeNotifierProvider<RealtimeDataService>.value(
      value: _realtimeService!,
      child: Consumer<RealtimeDataService>(
        builder: (context, realtimeService, child) {
          final isConnected = realtimeService.isConnected;
          
          return FloatingActionButton.extended(
            onPressed: () async {
              if (isConnected) {
                await realtimeService.stopService();
              } else {
                await realtimeService.startService();
              }
            },
            icon: Icon(
              isConnected ? Icons.stop : Icons.play_arrow,
              color: Colors.white,
            ),
            label: Text(
              isConnected ? 'Stop Live Data' : 'Start Live Data',
              style: const TextStyle(color: Colors.white),
            ),
            backgroundColor: isConnected 
                ? const Color(0xFFFF1493)  // Neon pink when connected
                : Colors.grey[600],
            tooltip: isConnected 
                ? 'Stop real-time data and cursor updates'
                : 'Start real-time data simulation and cursor tracking',
          );
        },
      ),
    );
  }

  void _loadECUDefinition(INIECUManager ecuManager) async {
    try {
      FilePickerResult? result = await FilePicker.platform.pickFiles(
        type: FileType.custom,
        allowedExtensions: ['ini'],
        dialogTitle: 'Load ECU Definition (INI File)',
      );

      if (result != null && result.files.single.path != null) {
        final success = await ecuManager.loadECUDefinition(result.files.single.path!);
        
        if (success) {
          // Set as current ECU (assuming single ECU for now)
          final firstECU = ecuManager.ecuDefinitions.keys.first;
          ecuManager.setCurrentECU(firstECU);
          
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text('✅ ECU definition loaded: $firstECU'),
              duration: const Duration(seconds: 2),
            ),
          );
        } else {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('❌ Failed to load ECU definition'),
              backgroundColor: Colors.red,
              duration: Duration(seconds: 3),
            ),
          );
        }
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Error loading ECU definition: $e'),
          backgroundColor: Colors.red,
          duration: const Duration(seconds: 3),
        ),
      );
    }
  }

  void _createNewFile(INIMSQFileService msqService) {
    try {
      msqService.createNewFile();
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('🆕 New tune file created with default values'),
          duration: Duration(seconds: 2),
        ),
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Error creating new file: $e'),
          backgroundColor: Colors.red,
          duration: const Duration(seconds: 3),
        ),
      );
    }
  }

  void _loadFile(INIMSQFileService msqService) async {
    try {
      FilePickerResult? result = await FilePicker.platform.pickFiles(
        type: FileType.custom,
        allowedExtensions: ['msq', 'json'],
        dialogTitle: 'Load Tune File',
      );

      if (result != null && result.files.single.path != null) {
        final success = await msqService.loadFile(result.files.single.path!);
        
        if (success) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('📂 Tune file loaded successfully'),
              duration: Duration(seconds: 2),
            ),
          );
        } else {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('❌ Failed to load tune file'),
              backgroundColor: Colors.red,
              duration: Duration(seconds: 3),
            ),
          );
        }
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Error loading file: $e'),
          backgroundColor: Colors.red,
          duration: const Duration(seconds: 3),
        ),
      );
    }
  }

  void _saveFile(INIMSQFileService msqService) async {
    bool success;
    
    if (msqService.currentFilePath != null) {
      success = await msqService.saveFile();
    } else {
      // Save as new file
      String? outputFile = await FilePicker.platform.saveFile(
        dialogTitle: 'Save Tune File',
        fileName: 'tune.msq',
        allowedExtensions: ['msq'],
      );

      if (outputFile != null) {
        success = await msqService.saveFile(outputFile);
      } else {
        return; // User cancelled
      }
    }

    if (success) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('💾 Tune file saved successfully'),
          duration: Duration(seconds: 2),
        ),
      );
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('❌ Failed to save tune file'),
          backgroundColor: Colors.red,
          duration: Duration(seconds: 3),
        ),
      );
    }
  }
}