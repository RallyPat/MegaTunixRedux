/// Persistent Navigation Layout
/// Provides consistent hamburger menu and navigation across all screens
/// Supports pinning the navigation drawer

import 'package:flutter/material.dart';
import 'ecu_navigation_drawer.dart';

/// Base layout that provides persistent navigation
class PersistentNavigationLayout extends StatefulWidget {
  final Widget child;
  final String title;
  final List<Widget>? actions;
  final bool showBackButton;
  final Widget? floatingActionButton;

  const PersistentNavigationLayout({
    super.key,
    required this.child,
    required this.title,
    this.actions,
    this.showBackButton = true,
    this.floatingActionButton,
  });

  @override
  State<PersistentNavigationLayout> createState() => _PersistentNavigationLayoutState();
}

class _PersistentNavigationLayoutState extends State<PersistentNavigationLayout> {
  bool _isDrawerPinned = false;
  final GlobalKey<ScaffoldState> _scaffoldKey = GlobalKey<ScaffoldState>();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      key: _scaffoldKey,
      backgroundColor: Theme.of(context).colorScheme.surface,
      
      // App bar with persistent hamburger menu
      appBar: AppBar(
        title: Text(widget.title),
        // Use a single leading widget to avoid overflow
        leading: IconButton(
          icon: const Icon(Icons.menu),
          onPressed: () {
            _scaffoldKey.currentState?.openDrawer();
          },
          tooltip: 'Open Navigation',
        ),
        // Move back button and pin toggle to actions area
        actions: [
          // Back button (when requested)
          if (widget.showBackButton)
            IconButton(
              icon: const Icon(Icons.arrow_back),
              onPressed: () {
                // Navigate back after current frame
                WidgetsBinding.instance.addPostFrameCallback((_) {
                  Navigator.of(context).pop();
                });
              },
              tooltip: 'Go Back',
            ),
          // Navigation drawer pin toggle
          IconButton(
            icon: Icon(
              _isDrawerPinned ? Icons.star : Icons.star_border,
              color: _isDrawerPinned 
                ? Theme.of(context).colorScheme.primary 
                : Theme.of(context).colorScheme.onSurface,
            ),
            onPressed: () {
              setState(() {
                _isDrawerPinned = !_isDrawerPinned;
              });
            },
            tooltip: _isDrawerPinned ? 'Unpin Navigation' : 'Pin Navigation',
          ),
          // Add any additional actions
          if (widget.actions != null) ...widget.actions!,
        ],
      ),
      
      // Always show the navigation drawer
      drawer: const ECUNavigationDrawer(),
      
      // Main content with optional pinned drawer
      body: Row(
        children: [
          // Pinned navigation drawer (when enabled)
          if (_isDrawerPinned)
            Container(
              width: 280,
              child: const ECUNavigationDrawer(),
            ),
          
          // Main content area
          Expanded(
            child: widget.child,
          ),
        ],
      ),
      
      // Optional floating action button
      floatingActionButton: widget.floatingActionButton,
    );
  }
}
