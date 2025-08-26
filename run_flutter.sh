#!/bin/bash

# MegaTunix Redux Flutter Development Script
# Provides convenient commands using the local Flutter installation

FLUTTER_DIR="/home/pat/Documents/GitHubRepos/MegaTunixRedux/flutter"
PROJECT_DIR="/home/pat/Documents/GitHubRepos/MegaTunixRedux/megatunix_flutter"
FLUTTER_BIN="$FLUTTER_DIR/bin/flutter"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}🚗 MegaTunix Redux Flutter Development Tools${NC}"
echo -e "${BLUE}============================================${NC}"
echo

# Check if Flutter exists
if [ ! -f "$FLUTTER_BIN" ]; then
    echo -e "${RED}❌ Flutter not found at: $FLUTTER_BIN${NC}"
    exit 1
fi

# Change to project directory
cd "$PROJECT_DIR" || {
    echo -e "${RED}❌ Project directory not found: $PROJECT_DIR${NC}"
    exit 1
}

# Show available commands if no argument provided
if [ $# -eq 0 ]; then
    echo -e "${YELLOW}Available commands:${NC}"
    echo -e "  ${GREEN}run${NC}        - Run the app in development mode"
    echo -e "  ${GREEN}run-release${NC} - Run the app in release mode"
    echo -e "  ${GREEN}build${NC}      - Build the app for production"
    echo -e "  ${GREEN}clean${NC}      - Clean build files and get dependencies"
    echo -e "  ${GREEN}analyze${NC}    - Analyze code for issues"
    echo -e "  ${GREEN}test${NC}       - Run tests"
    echo -e "  ${GREEN}doctor${NC}     - Check Flutter installation"
    echo
    echo -e "${YELLOW}Usage:${NC} $0 <command>"
    echo -e "${YELLOW}Example:${NC} $0 run"
    exit 0
fi

case $1 in
    "run")
        echo -e "${GREEN}🚀 Running MegaTunix Redux in development mode...${NC}"
        echo -e "${BLUE}📋 Available devices:${NC}"
        "$FLUTTER_BIN" devices
        echo
        echo -e "${BLUE}🎯 Starting app on Linux desktop...${NC}"
        "$FLUTTER_BIN" run -d linux
        ;;
    "run-release")
        echo -e "${GREEN}🚀 Running MegaTunix Redux in release mode...${NC}"
        "$FLUTTER_BIN" run -d linux --release
        ;;
    "build")
        echo -e "${GREEN}🔨 Building MegaTunix Redux for Linux...${NC}"
        "$FLUTTER_BIN" build linux --release
        echo -e "${GREEN}✅ Build complete! Binary location:${NC}"
        echo -e "${BLUE}   $PROJECT_DIR/build/linux/x64/release/bundle/${NC}"
        ;;
    "clean")
        echo -e "${GREEN}🧹 Cleaning project and getting dependencies...${NC}"
        "$FLUTTER_BIN" clean
        "$FLUTTER_BIN" pub get
        echo -e "${GREEN}✅ Project cleaned and dependencies updated${NC}"
        ;;
    "analyze")
        echo -e "${GREEN}🔍 Analyzing code...${NC}"
        "$FLUTTER_BIN" analyze --no-fatal-infos
        ;;
    "test")
        echo -e "${GREEN}🧪 Running tests...${NC}"
        "$FLUTTER_BIN" test
        ;;
    "doctor")
        echo -e "${GREEN}🏥 Checking Flutter installation...${NC}"
        "$FLUTTER_BIN" doctor -v
        ;;
    *)
        echo -e "${RED}❌ Unknown command: $1${NC}"
        echo -e "${YELLOW}Run '$0' without arguments to see available commands${NC}"
        exit 1
        ;;
esac