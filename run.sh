set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

source .venv/bin/activate

# Rebuild the C++ app if any source files changed
make -C src

python src/model/model.py &
MODEL_PID=$!
trap 'kill "$MODEL_PID" 2>/dev/null' EXIT

cd src
./demo_app

exit