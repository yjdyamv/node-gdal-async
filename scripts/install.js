'use strict'

const { execSync } = require('child_process')
const path = require('path')
const fs = require('fs')

// Create self-referencing symlink so require('@yjdyamv/gdal-async') resolves
const projectRoot = path.resolve(__dirname, '..')
const scopedDir = path.join(projectRoot, 'node_modules', '@yjdyamv')
const linkPath = path.join(scopedDir, 'gdal-async')
if (!fs.existsSync(linkPath)) {
  fs.mkdirSync(scopedDir, { recursive: true })
  try {
    if (process.platform === 'win32') {
      require('child_process').execSync(`cmd /c mklink /J "${linkPath}" "${projectRoot}"`, { stdio: 'ignore' })
    } else {
      fs.symlinkSync(projectRoot, linkPath, 'junction')
    }
  } catch (_) { /* already exists or not supported */ }
}

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

// Find vcpkg toolchain
function findVcpkgToolchain() {
  const root = process.env.VCPKG_ROOT
  if (root) {
    const candidate = path.join(root, 'scripts', 'buildsystems', 'vcpkg.cmake')
    if (fs.existsSync(candidate)) return candidate
  }
  // Common vcpkg install locations
  const candidates = [
    path.resolve('D:/vcpkg/scripts/buildsystems/vcpkg.cmake'),
  ]
  for (const c of candidates) {
    if (fs.existsSync(c)) return c
  }
  return null
}

const vcpkgToolchain = findVcpkgToolchain()

// Build from source with cmake-js
const buildArgs = []
if (process.env.npm_config_j) {
  buildArgs.push('-j', process.env.npm_config_j)
}
const env = { ...process.env }
if (vcpkgToolchain) {
  buildArgs.push('--CDCMAKE_TOOLCHAIN_FILE=' + vcpkgToolchain)
  console.log('[gdal-async] Using vcpkg toolchain:', vcpkgToolchain)
} else {
  console.log('[gdal-async] vcpkg not found, assuming system-installed GDAL')
}

try {
  const args = ['cmake-js', 'compile', ...buildArgs].join(' ')
  execSync(`npx ${args}`, {
    stdio: 'inherit',
    env: env
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
