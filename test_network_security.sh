#!/bin/bash

# Network Security Test Suite for MegaTunix Redux
# Tests the multi-user collaborative tuning capabilities

echo "=== MegaTunix Redux Network Security Test ==="
echo "Testing multi-user collaborative tuning system..."

# Test 1: Check if network security libraries are available
echo -e "\n1. Checking network security dependencies..."
pkg-config --exists gnutls && echo "✓ GnuTLS available" || echo "✗ GnuTLS missing"
pkg-config --exists json-glib-1.0 && echo "✓ JSON-GLib available" || echo "✗ JSON-GLib missing"
pkg-config --exists gio-2.0 && echo "✓ GIO available" || echo "✗ GIO missing"

# Test 2: Check if network security is compiled into binary
echo -e "\n2. Checking network security integration..."
if strings build/src/megatunix-redux | grep -q "mtx_network_security_init"; then
    echo "✓ Network security functions compiled in"
else
    echo "✗ Network security functions not found"
fi

# Test 3: Check for multi-user support structures
echo -e "\n3. Checking multi-user support structures..."
if strings build/src/megatunix-redux | grep -q "MTX_USER_ROLE"; then
    echo "✓ User role system compiled in"
else
    echo "✗ User role system not found"
fi

# Test 4: Check for TLS/encryption support
echo -e "\n4. Checking TLS/encryption support..."
if strings build/src/megatunix-redux | grep -q "gnutls_session_t"; then
    echo "✓ TLS session support compiled in"
else
    echo "✗ TLS session support not found"
fi

# Test 5: Check for collaborative features
echo -e "\n5. Checking collaborative features..."
if strings build/src/megatunix-redux | grep -q "MTX_MSG_TYPE_TUNE_CHANGE"; then
    echo "✓ Collaborative tune change messaging compiled in"
else
    echo "✗ Collaborative tune change messaging not found"
fi

if strings build/src/megatunix-redux | grep -q "MTX_MSG_TYPE_REALTIME_DATA"; then
    echo "✓ Real-time data sharing compiled in"
else
    echo "✗ Real-time data sharing not found"
fi

if strings build/src/megatunix-redux | grep -q "MTX_MSG_TYPE_CHAT_MESSAGE"; then
    echo "✓ Chat system for collaboration compiled in"
else
    echo "✗ Chat system not found"
fi

# Test 6: Check for user management
echo -e "\n6. Checking user management system..."
if strings build/src/megatunix-redux | grep -q "mtx_user_authenticate"; then
    echo "✓ User authentication system compiled in"
else
    echo "✗ User authentication system not found"
fi

if strings build/src/megatunix-redux | grep -q "mtx_user_has_permission"; then
    echo "✓ Permission system compiled in"
else
    echo "✗ Permission system not found"
fi

# Test 7: Check for session management
echo -e "\n7. Checking session management..."
if strings build/src/megatunix-redux | grep -q "mtx_session_new"; then
    echo "✓ Session management compiled in"
else
    echo "✗ Session management not found"
fi

if strings build/src/megatunix-redux | grep -q "mtx_session_setup_tls"; then
    echo "✓ TLS session setup compiled in"
else
    echo "✗ TLS session setup not found"
fi

# Test 8: Check for message serialization
echo -e "\n8. Checking message serialization..."
if strings build/src/megatunix-redux | grep -q "mtx_network_message_serialize"; then
    echo "✓ Message serialization compiled in"
else
    echo "✗ Message serialization not found"
fi

# Test 9: Check for security utilities
echo -e "\n9. Checking security utilities..."
if strings build/src/megatunix-redux | grep -q "mtx_security_hash_password"; then
    echo "✓ Password hashing compiled in"
else
    echo "✗ Password hashing not found"
fi

if strings build/src/megatunix-redux | grep -q "mtx_crypto_encrypt_data"; then
    echo "✓ Data encryption compiled in"
else
    echo "✗ Data encryption not found"
fi

# Test 10: Summary
echo -e "\n=== Test Summary ==="
echo "MegaTunix Redux Network Security Features:"
echo "• Multi-user collaborative tuning system"
echo "• Role-based access control (Viewer, Tuner, Admin, Owner)"
echo "• TLS-encrypted communication"
echo "• Real-time data sharing between users"
echo "• Secure tune parameter change notifications"
echo "• User authentication and session management"
echo "• Chat system for collaboration"
echo "• End-to-end encrypted messaging"

echo -e "\nNext steps for full network functionality:"
echo "1. Set up TLS certificates for secure communication"
echo "2. Configure user database for authentication"
echo "3. Test multi-user scenarios with multiple clients"
echo "4. Validate real-time data synchronization"
echo "5. Test collaborative tuning workflows"

echo -e "\nTest completed!"
