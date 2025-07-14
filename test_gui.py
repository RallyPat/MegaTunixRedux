#!/usr/bin/env python3
"""
Simple test script to verify MegaTunix Redux GUI functionality
"""

import os
import sys
import subprocess
import time
import signal

def test_gui_startup():
    """Test that the GUI starts up without fatal errors"""
    print("Testing MegaTunix Redux GUI startup...")
    
    # Set up test environment
    test_home = "/tmp/megatunix-test"
    os.environ["HOME"] = test_home
    
    # Path to the binary
    binary_path = "/home/pat/Documents/Github Repos/MegaTunixRedux/build/src/megatunix-redux"
    
    if not os.path.exists(binary_path):
        print(f"ERROR: Binary not found at {binary_path}")
        return False
    
    # Start the application
    print("Starting MegaTunix Redux...")
    try:
        process = subprocess.Popen(
            [binary_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid
        )
        
        # Give it time to start
        time.sleep(3)
        
        # Check if it's still running
        if process.poll() is None:
            print("✓ Application started successfully")
            
            # Give it a bit more time to fully initialize
            time.sleep(2)
            
            # Terminate the process
            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
            process.wait(timeout=5)
            
            print("✓ Application terminated cleanly")
            return True
        else:
            stdout, stderr = process.communicate()
            print(f"✗ Application failed to start")
            print(f"stdout: {stdout}")
            print(f"stderr: {stderr}")
            return False
            
    except Exception as e:
        print(f"✗ Error starting application: {e}")
        return False

def test_help_output():
    """Test that help output works"""
    print("Testing help output...")
    
    binary_path = "/home/pat/Documents/Github Repos/MegaTunixRedux/build/src/megatunix-redux"
    test_home = "/tmp/megatunix-test"
    
    try:
        env = os.environ.copy()
        env["HOME"] = test_home
        
        result = subprocess.run(
            [binary_path, "--help"],
            capture_output=True,
            text=True,
            env=env,
            timeout=10
        )
        
        if result.returncode == 0 and "MegaTunix" in result.stdout:
            print("✓ Help output working correctly")
            return True
        else:
            print("✗ Help output failed")
            print(f"Return code: {result.returncode}")
            print(f"stdout: {result.stdout}")
            print(f"stderr: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"✗ Error testing help: {e}")
        return False

def main():
    """Run all tests"""
    print("MegaTunix Redux GUI Test Suite")
    print("=" * 40)
    
    tests = [
        ("Help Output", test_help_output),
        ("GUI Startup", test_gui_startup),
    ]
    
    passed = 0
    total = len(tests)
    
    for test_name, test_func in tests:
        print(f"\n{test_name}:")
        if test_func():
            passed += 1
        else:
            print(f"Failed: {test_name}")
    
    print(f"\n" + "=" * 40)
    print(f"Test Results: {passed}/{total} passed")
    
    if passed == total:
        print("✓ All tests passed!")
        return 0
    else:
        print("✗ Some tests failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
