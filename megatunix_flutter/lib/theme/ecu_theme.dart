import 'package:flutter/material.dart';

/// Optimized automotive theme system for MegaTunix Redux
/// Provides beautiful, modern appearance for ECU tuning software
class ECUTheme {
  // Private constructor to prevent instantiation
  ECUTheme._();

  // Core color palette - optimized for performance
  static const Color primaryBlue = Color(0xFF42CEDB);
  static const Color primaryBlueDark = Color(0xFF3BB8C4);
  static const Color accentOrange = Color(0xFFFF6B35);
  static const Color accentGreen = Color(0xFF4CAF50);
  static const Color accentRed = Color(0xFFF44336);
  static const Color accentYellow = Color(0xFFFFC107);

  // Dark theme color palette
  static const Color backgroundDark = Color(0xFF1A1A1A);
  static const Color surfaceDark = Color(0xFF2D2D2D);
  static const Color cardDark = Color(0xFF3A3A3A);
  static const Color borderDark = Color(0xFF404040);
  static const Color textPrimary = Color(0xFFFFFFFF);
  static const Color textSecondary = Color(0xFFB3B3B3);
  static const Color textDisabled = Color(0xFF666666);

  // Light theme color palette
  static const Color backgroundLight = Color(0xFFFAFAFA);
  static const Color surfaceLight = Color(0xFFFFFFFF);
  static const Color cardLight = Color(0xFFF5F5F5);
  static const Color borderLight = Color(0xFFE0E0E0);

  /// Dark theme - Professional automotive appearance
  static final ThemeData darkTheme = ThemeData(
    useMaterial3: true,
    brightness: Brightness.dark,

    // Color scheme - using extracted constants
    colorScheme: const ColorScheme.dark(
      primary: primaryBlue,
      primaryContainer: primaryBlueDark,
      secondary: accentOrange,
      secondaryContainer: accentOrange,
      surface: surfaceDark,
      error: accentRed,
      onPrimary: textPrimary,
      onSecondary: textPrimary,
      onSurface: textPrimary,
      onError: textPrimary,
    ),
  );

  /// Light theme - Alternative appearance
  static final ThemeData lightTheme = ThemeData(
    useMaterial3: true,
    brightness: Brightness.light,

    // Color scheme - using extracted constants
    colorScheme: const ColorScheme.light(
      primary: primaryBlue,
      primaryContainer: primaryBlueDark,
      secondary: accentOrange,
      secondaryContainer: accentOrange,
      surface: surfaceLight,
      error: accentRed,
      onPrimary: textPrimary,
      onSecondary: textPrimary,
      onSurface: textPrimary,
      onError: textPrimary,
    ),
  );

  /// Get accent color for specific ECU parameters
  static Color getAccentColor(String parameter) {
    const Map<String, Color> accentMap = {
      'rpm': primaryBlue,
      'map': accentOrange,
      'tps': accentYellow,
      'afr': accentGreen,
      'timing': primaryBlue,
      'boost': accentRed,
      'battery': accentGreen,
      'temp': accentOrange,
      'primary': primaryBlue,
      'secondary': accentOrange,
      'info': primaryBlue,
      'success': accentGreen,
      'error': accentRed,
      'warning': accentYellow,
      'selection': accentYellow,
      'edit': accentYellow,
      'low': primaryBlue,
      'medium': accentGreen,
      'high': accentRed,
    };

    return accentMap[parameter.toLowerCase()] ?? primaryBlue;
  }
}
