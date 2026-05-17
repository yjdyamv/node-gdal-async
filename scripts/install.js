'use strict'

const { execSync } = require('child_process')
const path = require('path')
const fs = require('fs')

const bindingDir = path.resolve(__dirname, '..', 'lib', 'binding')

// Check if gdal.node already exists in lib/binding (from a previous build or prebuilt download)
function findExistingBinding() {
  if (!fs.existsSync(bindingDir)) return null
  const dirs = fs.readdirSync(bindingDir)
  for (const dir of dirs) {
    const candidate = path.join(bindingDir, dir, 'gdal.node')
    if (fs.existsSync(candidate)) return candidate
  }
  return null
}

const existing = findExistingBinding()
if (existing) {
  console.log('[gdal-async] Found existing gdal.node at', existing)
  process.exit(0)
}

// Try downloading a prebuilt binary
console.log('[gdal-async] Trying prebuilt binary...')
try {
  execSync('npx node-pre-gyp install --fallback-to-build=false', {
    stdio: 'inherit',
    env: { ...process.env }
  })
  const prebuilt = findExistingBinding()
  if (prebuilt) {
    console.log('[gdal-async] Prebuilt binary installed successfully.')
    process.exit(0)
  }
} catch (err) {
  console.log('[gdal-async] No prebuilt binary available, building from source...')
}

// Build from source with cmake-js
const buildArgs = []
if (process.env.npm_config_build_from_source || process.env.npm_config_build_from_source === 'true') {
  // User explicitly requested source build
}
if (process.env.npm_config_j) {
  buildArgs.push('-j', process.env.npm_config_j)
}

try {
  const args = ['cmake-js', 'compile', ...buildArgs].join(' ')
  execSync(`npx ${args}`, {
    stdio: 'inherit',
    env: { ...process.env }
  })
} catch (err) {
  console.error('[gdal-async] cmake-js build failed:', err.message)
  process.exit(1)
}

// cmake-js outputs to build/Release/gdal.node
// The CMakeLists.txt post-build step copies it to lib/binding/
const finalBinding = findExistingBinding()
if (finalBinding) {
  console.log('[gdal-async] Build succeeded, gdal.node at', finalBinding)
} else {
  console.error('[gdal-async] Build completed but gdal.node not found.')
  process.exit(1)
}
