#!/bin/bash
# setup-vcpkg-host.sh - Setup ottimizzato di vcpkg sull'host per build Docker velocissime

set -e

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Funzioni per log colorati
log_info() { echo -e "${BLUE}ℹ️  $1${NC}"; }
log_success() { echo -e "${GREEN}✅ $1${NC}"; }
log_warning() { echo -e "${YELLOW}⚠️  $1${NC}"; }
log_error() { echo -e "${RED}❌ $1${NC}"; }
log_step() { echo -e "${CYAN}🔧 $1${NC}"; }

# Banner
echo -e "${CYAN}"
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║          vcpkg Host Setup for Ultra-Fast Docker Builds    ║"
echo "║                    STL2GLB Converter v2.0                 ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Chiedi directory per vcpkg (default: /home/vcpkg)
DEFAULT_DIR="/home/vcpkg"
read -p "📁 Directory per vcpkg [$DEFAULT_DIR]: " VCPKG_DIR
VCPKG_DIR=${VCPKG_DIR:-$DEFAULT_DIR}

# Crea directory se non esiste
if [ ! -d "$VCPKG_DIR" ]; then
    log_step "Creating directory $VCPKG_DIR..."
    sudo mkdir -p "$VCPKG_DIR"
    sudo chown $USER:$USER "$VCPKG_DIR"
fi

cd "$VCPKG_DIR"

# Controlla se vcpkg è già installato
if [ -f "vcpkg" ]; then
    log_success "vcpkg already installed in $VCPKG_DIR"

    # Chiedi se aggiornare
    read -p "🔄 Update vcpkg? (y/N): " UPDATE_VCPKG
    if [[ "$UPDATE_VCPKG" =~ ^[Yy]$ ]]; then
        log_step "Updating vcpkg..."
        git pull
        ./bootstrap-vcpkg.sh -disableMetrics
    fi
else
    log_step "Installing vcpkg..."

    # Installa dipendenze di sistema necessarie
    log_info "Installing system dependencies..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y --no-install-recommends \
            build-essential git cmake ninja-build pkg-config \
            curl zip unzip tar ca-certificates \
            autoconf automake autoconf-archive \
            libtool libssl-dev
    elif command -v yum &> /dev/null; then
        sudo yum install -y \
            gcc gcc-c++ git cmake ninja-build \
            curl zip unzip tar ca-certificates \
            autoconf automake libtool openssl-devel
    else
        log_warning "Unsupported package manager. Please install build tools manually."
    fi

    # Clona vcpkg
    log_step "Cloning vcpkg repository..."
    git clone --depth 1 https://github.com/Microsoft/vcpkg.git .

    # Bootstrap vcpkg
    log_step "Bootstrapping vcpkg..."
    ./bootstrap-vcpkg.sh -disableMetrics

    log_success "vcpkg installed successfully!"
fi

# Imposta variabili d'ambiente per performance
export VCPKG_MAX_CONCURRENCY=$(nproc)
export MAKEFLAGS="-j$(nproc)"
export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc)

log_info "Using $VCPKG_MAX_CONCURRENCY parallel jobs for builds"

# Installa le librerie necessarie
log_step "Installing C++ dependencies for STL2GLB..."

# Lista delle librerie richieste
LIBS=(
    "openssl"
    "nlohmann-json"
    "cpp-httplib"
    "draco"
)

# Determina il triplet (Linux x64)
TRIPLET="x64-linux"
if [[ "$OSTYPE" == "darwin"* ]]; then
    TRIPLET="x64-osx"
fi

log_info "Using triplet: $TRIPLET"

# Installa ogni libreria
for LIB in "${LIBS[@]}"; do
    if [ -d "installed/$TRIPLET/include" ] && ls installed/$TRIPLET/include/*${LIB}* 1> /dev/null 2>&1; then
        log_success "$LIB already installed"
    else
        log_step "Installing $LIB..."
        ./vcpkg install $LIB:$TRIPLET
    fi
done

# Crea script di ambiente
ENV_SCRIPT="$VCPKG_DIR/vcpkg-env.sh"
cat > "$ENV_SCRIPT" << EOF
#!/bin/bash
# vcpkg environment setup for STL2GLB

export VCPKG_ROOT="$VCPKG_DIR"
export PATH="\$VCPKG_ROOT:\$PATH"
export CMAKE_TOOLCHAIN_FILE="\$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
export VCPKG_TRIPLET="$TRIPLET"

echo "✅ vcpkg environment loaded from $VCPKG_DIR"
EOF

chmod +x "$ENV_SCRIPT"

# Mostra statistiche
echo ""
log_info "📊 Installation Summary:"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
log_success "vcpkg directory: $VCPKG_DIR"
log_success "Installed libraries:"
for LIB in "${LIBS[@]}"; do
    echo "  • $LIB"
done
log_success "Triplet: $TRIPLET"
log_success "Environment script: $ENV_SCRIPT"

# Calcola spazio usato
SPACE_USED=$(du -sh "$VCPKG_DIR" 2>/dev/null | cut -f1)
log_info "Total space used: $SPACE_USED"

echo ""
echo -e "${GREEN}🎉 Setup completed successfully!${NC}"
echo ""
echo "To use vcpkg in your shell:"
echo -e "${YELLOW}  source $ENV_SCRIPT${NC}"
echo ""
echo "To use with Docker Compose:"
echo -e "${YELLOW}  VCPKG_HOST_DIR=$VCPKG_DIR docker-compose -f docker-compose-optimized.yml up --build${NC}"
echo ""

# Opzionale: aggiungi al .bashrc
read -p "Add vcpkg to your .bashrc? (y/N): " ADD_TO_BASHRC
if [[ "$ADD_TO_BASHRC" =~ ^[Yy]$ ]]; then
    echo "" >> ~/.bashrc
    echo "# vcpkg for STL2GLB" >> ~/.bashrc
    echo "source $ENV_SCRIPT" >> ~/.bashrc
    log_success "Added to ~/.bashrc - restart shell or run: source ~/.bashrc"
fi