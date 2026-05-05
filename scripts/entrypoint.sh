#!/usr/bin/env bash
set -euo pipefail

mkdir -p "${OUT_DIR:-/out}"

: "${SRC_DIR:=/src}"
: "${OUT_DIR:=/out}"
: "${BUILD_DIR:=/work/build}"
: "${APPDIR:=/work/AppDir}"
: "${APP_TARGET:?Set APP_TARGET to your executable target name (for example: MyApp)}"
: "${APP_NAME:=$APP_TARGET}"
: "${APP_VERSION:=snapshot}"
: "${CMAKE_BUILD_TYPE:=Release}"
: "${CMAKE_GENERATOR:=Ninja}"

APPIMAGE_TOOL_ARCH="${APPIMAGE_TOOL_ARCH:-x86_64}"
APPIMAGE_TOOL_URL="${APPIMAGE_TOOL_URL:-https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${APPIMAGE_TOOL_ARCH}.AppImage}"
APPIMAGE_TOOL="${APPIMAGE_TOOL:-${BUILD_DIR}/appimagetool-${APPIMAGE_TOOL_ARCH}.AppImage}"
SYSROOT="${SYSROOT:-/}"
APP_LIB_DIR="${APPDIR}/usr/lib/aarch64-linux-gnu"

declare -A BUNDLED_LIBS=()

is_system_library() {
  case "$1" in
    libc.so.*|libm.so.*|libpthread.so.*|libdl.so.*|librt.so.*|libanl.so.*|libresolv.so.*|libutil.so.*|libcrypt.so.*|ld-linux-*.so.*|linux-vdso.so.*)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

find_library_in_sysroot() {
  local library_name="$1"
  local search_dir found_path

  for search_dir in \
    "${SYSROOT}/usr/lib/aarch64-linux-gnu" \
    "${SYSROOT}/lib/aarch64-linux-gnu" \
    "${SYSROOT}/usr/lib" \
    "${SYSROOT}/lib"; do
    if [[ -d "$search_dir" ]]; then
      found_path="$(find "$search_dir" -name "${library_name}" 2>/dev/null | head -n 1)"
      if [[ -n "$found_path" ]]; then
        printf '%s\n' "$found_path"
        return 0
      fi
    fi
  done

  return 0
}

find_qt_tree() {
  local tree_name="$1"
  local qt_root

  for qt_root in \
    "${SYSROOT}/usr/lib/aarch64-linux-gnu/qt6" \
    "${SYSROOT}/usr/lib/qt6"; do
    if [[ -d "${qt_root}/${tree_name}" ]]; then
      printf '%s\n' "${qt_root}/${tree_name}"
      return 0
    fi
  done

  return 0
}

bundle_library_recursive() {
  local source_path="$1"
  local output_dir="$2"
  local library_name dest_path dependency resolved_dependency

  library_name="$(basename "$source_path")"
  if [[ -n "${BUNDLED_LIBS[${library_name}]:-}" ]]; then
    return 0
  fi
  BUNDLED_LIBS["${library_name}"]=1

  mkdir -p "$output_dir"
  dest_path="${output_dir}/${library_name}"
  cp -L "$source_path" "$dest_path"

  while IFS= read -r dependency; do
    [[ -z "$dependency" ]] && continue
    if is_system_library "$dependency"; then
      continue
    fi

    resolved_dependency="$(find_library_in_sysroot "$dependency")"
    if [[ -n "$resolved_dependency" ]]; then
      bundle_library_recursive "$resolved_dependency" "$output_dir"
    else
      echo "  ? missing dependency in sysroot: $dependency" >&2
    fi
  done < <(patchelf --print-needed "$dest_path")
}

bundle_binary_dependencies() {
  local binary_path="$1"
  local dependency resolved_dependency

  while IFS= read -r dependency; do
    [[ -z "$dependency" ]] && continue
    if is_system_library "$dependency"; then
      continue
    fi

    resolved_dependency="$(find_library_in_sysroot "$dependency")"
    if [[ -n "$resolved_dependency" ]]; then
      bundle_library_recursive "$resolved_dependency" "$APP_LIB_DIR"
    else
      echo "  ? missing dependency in sysroot: $dependency" >&2
    fi
  done < <(patchelf --print-needed "$binary_path")
}

bundle_directory_dependencies() {
  local root_dir="$1"
  local output_dir="${2:-$APP_LIB_DIR}"

  while IFS= read -r -d '' library; do
    bundle_binary_dependencies "$library"
  done < <(find "$root_dir" -type f -name "*.so" -print0 2>/dev/null)
}

copy_qt_tree() {
  local source_dir="$1"
  local destination_dir="$2"

  if [[ -d "$source_dir" ]]; then
    rm -rf "$destination_dir"
    mkdir -p "$(dirname "$destination_dir")"
    cp -aL "$source_dir" "$destination_dir"
  fi
}

rm -rf "${BUILD_DIR}" "${APPDIR}"
mkdir -p "${BUILD_DIR}" "${APPDIR}" "${OUT_DIR}"

echo "[1/5] Configuring CMake"
cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" -G "${CMAKE_GENERATOR}" \
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
  -DCMAKE_INSTALL_PREFIX=/usr

echo "[2/5] Building ${APP_TARGET}"
cmake --build "${BUILD_DIR}" --config "${CMAKE_BUILD_TYPE}" --target "${APP_TARGET}"

echo "[3/5] Installing into AppDir"
cmake --install "${BUILD_DIR}" --config "${CMAKE_BUILD_TYPE}" --prefix "${APPDIR}/usr"

echo "[3/5] Bundling runtime dependencies"
bundle_binary_dependencies "${APPDIR}/usr/bin/${APP_TARGET}"

QT_PLUGIN_SOURCE="$(find_qt_tree plugins)"
QT_QML_SOURCE="$(find_qt_tree qml)"

if [[ -n "${QT_PLUGIN_SOURCE}" ]]; then
  copy_qt_tree "${QT_PLUGIN_SOURCE}" "${APP_LIB_DIR}/qt6/plugins"
  bundle_directory_dependencies "${APP_LIB_DIR}/qt6/plugins"
fi

if [[ -n "${QT_QML_SOURCE}" ]]; then
  copy_qt_tree "${QT_QML_SOURCE}" "${APP_LIB_DIR}/qt6/qml"
  bundle_directory_dependencies "${APP_LIB_DIR}/qt6/qml"
fi

# A minimal launcher. Qt deployment normally drops qt.conf next to the executable,
# and this launcher keeps the AppImage portable.
cat > "${APPDIR}/AppRun" <<EOF
#!/bin/sh
HERE="\$(CDPATH= cd -- "\$(dirname -- "\$0")" && pwd)"
export LD_LIBRARY_PATH="\${HERE}/usr/lib/aarch64-linux-gnu\${LD_LIBRARY_PATH:+:\${LD_LIBRARY_PATH}}"
export QT_PLUGIN_PATH="\${HERE}/usr/lib/aarch64-linux-gnu/qt6/plugins\${QT_PLUGIN_PATH:+:\${QT_PLUGIN_PATH}}"
export QML2_IMPORT_PATH="\${HERE}/usr/lib/aarch64-linux-gnu/qt6/qml\${QML2_IMPORT_PATH:+:\${QML2_IMPORT_PATH}}"

if [ -z "\${QT_QPA_PLATFORM:-}" ]; then
  if [ -n "\${DISPLAY:-}" ]; then
    export QT_QPA_PLATFORM=xcb
  elif [ -n "\${WAYLAND_DISPLAY:-}" ]; then
    export QT_QPA_PLATFORM=wayland
  else
    export QT_QPA_PLATFORM=offscreen
  fi
fi

APPDIR="\$HERE" exec "\$HERE/usr/bin/${APP_TARGET}" "\$@"
EOF
chmod +x "${APPDIR}/AppRun"

# Desktop file for appimagetool.
cat > "${APPDIR}/${APP_NAME}.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=${APP_NAME}
Exec=${APP_TARGET}
Icon=${APP_NAME}
Categories=Utility;
Terminal=false
EOF

# Optional icon discovery. If none is found, appimagetool can still build, but
# the AppImage will have no custom icon.
if [ ! -f "${APPDIR}/${APP_NAME}.png" ]; then
  for candidate in \
    "${SRC_DIR}/assets/${APP_NAME}.png" \
    "${SRC_DIR}/assets/icon.png" \
    "${SRC_DIR}/resources/${APP_NAME}.png" \
    "${SRC_DIR}/resources/icon.png"; do
    if [ -f "$candidate" ]; then
      cp "$candidate" "${APPDIR}/${APP_NAME}.png"
      break
    fi
  done
fi

if [ ! -f "${APPDIR}/${APP_NAME}.png" ]; then
  # Tiny placeholder icon, only used if the project doesn't provide one.
  python3 - <<'PY' "${APPDIR}/${APP_NAME}.png"
from pathlib import Path
import base64
png = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAusB9Wl6p9kAAAAASUVORK5CYII="
)
Path(__import__("sys").argv[1]).write_bytes(png)
PY
fi

echo "[4/5] Fetching appimagetool (${APPIMAGE_TOOL_ARCH})"
if [ ! -x "${APPIMAGE_TOOL}" ]; then
  wget -qO "${APPIMAGE_TOOL}" "${APPIMAGE_TOOL_URL}"
  chmod +x "${APPIMAGE_TOOL}"
fi

OUTPUT="${OUT_DIR}/${APP_NAME}-${APP_VERSION}-aarch64.AppImage"

echo "[5/5] Creating AppImage: ${OUTPUT}"
ARCH=aarch64 "${APPIMAGE_TOOL}" --appimage-extract-and-run "${APPDIR}" "${OUTPUT}"

echo "Done."
echo "AppDir:   ${APPDIR}"
echo "AppImage: ${OUTPUT}"