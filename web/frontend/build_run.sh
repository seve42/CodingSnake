#!/usr/bin/env bash
set -euo pipefail

export NVM_DIR="$HOME/.nvm"
if [[ -s "$NVM_DIR/nvm.sh" ]]; then
  # Load nvm so node/npm are available
  . "$NVM_DIR/nvm.sh"
  nvm use 20.19.0 >/dev/null
fi

cd "$(dirname "$0")"

# Install dependencies if node_modules doesn't exist
if [ ! -d "node_modules" ]; then
  echo "Installing dependencies..."
  npm install
fi

npm run build
python3 serve_static.py --dir dist --port 5173
