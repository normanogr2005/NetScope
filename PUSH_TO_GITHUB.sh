#!/usr/bin/env bash
set -euo pipefail

REMOTE="https://github.com/normanogr2005/NetScope.git"

git remote remove origin 2>/dev/null || true
git remote add origin "$REMOTE"
git push -u origin main
