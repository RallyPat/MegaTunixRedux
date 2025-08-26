import 'package:flutter/material.dart';

void main() {
  runApp(const TestApp());
}

class TestApp extends StatelessWidget {
  const TestApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Window Test',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: const TestWindow(),
      debugShowCheckedModeBanner: false,
    );
  }
}

class TestWindow extends StatelessWidget {
  const TestWindow({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.red, // Make it very obvious
      appBar: AppBar(
        title: const Text('FLUTTER WINDOW TEST'),
        backgroundColor: Colors.green,
      ),
      body: Container(
        width: double.infinity,
        height: double.infinity,
        color: Colors.yellow,
        child: const Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                Icons.warning,
                size: 100,
                color: Colors.red,
              ),
              Text(
                'IF YOU SEE THIS',
                style: TextStyle(
                  fontSize: 48,
                  fontWeight: FontWeight.bold,
                  color: Colors.black,
                ),
              ),
              Text(
                'FLUTTER WINDOW IS WORKING!',
                style: TextStyle(
                  fontSize: 32,
                  fontWeight: FontWeight.bold,
                  color: Colors.black,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}