#-------------------------------------------------
#
# Project created by QtCreator 2014-05-15T16:31:50
#
#-------------------------------------------------

QT       -= gui

TARGET = xCairo
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ../../../

DEFINES += HAVE_CONFIG_H
#DEFINES += HAVE_LOCKDEP

QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter -Wno-enum-conversion -Wno-unused-variable -Wno-missing-field-initializers -Wno-unused-function -Wno-parentheses-equality
#QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-enum-conversion -Wno-unused-variable -Wno-missing-field-initializers -Wno-unused-function -Wno-parentheses-equality

SOURCES += \
    ../cairo.c \
    ../cairo-wideint.c \
    ../cairo-version.c \
    ../cairo-user-font.c \
    ../cairo-unicode.c \
    ../cairo-tristrip.c \
    ../cairo-traps.c \
    ../cairo-traps-compositor.c \
    ../cairo-toy-font-face.c \
    ../cairo-tor22-scan-converter.c \
    ../cairo-tor-scan-converter.c \
    ../cairo-time.c \
    ../cairo-surface.c \
    ../cairo-surface-wrapper.c \
    ../cairo-surface-subsurface.c \
    ../cairo-surface-snapshot.c \
    ../cairo-surface-offset.c \
    ../cairo-surface-observer.c \
    ../cairo-surface-fallback.c \
    ../cairo-surface-clipper.c \
    ../cairo-stroke-style.c \
    ../cairo-stroke-dash.c \
    ../cairo-spline.c \
    ../cairo-spans.c \
    ../cairo-spans-compositor.c \
    ../cairo-slope.c \
    ../cairo-shape-mask-compositor.c \
    ../cairo-scaled-font.c \
    ../cairo-scaled-font-subsets.c \
    ../cairo-rtree.c \
    ../cairo-region.c \
    ../cairo-rectangular-scan-converter.c \
    ../cairo-rectangle.c \
    ../cairo-recording-surface.c \
    ../cairo-raster-source-pattern.c \
    ../cairo-polygon.c \
    ../cairo-polygon-reduce.c \
    ../cairo-polygon-intersect.c \
    ../cairo-pen.c \
    ../cairo-pattern.c \
    ../cairo-path.c \
    ../cairo-path-stroke.c \
    ../cairo-path-stroke-tristrip.c \
    ../cairo-path-stroke-traps.c \
    ../cairo-path-stroke-polygon.c \
    ../cairo-path-stroke-boxes.c \
    ../cairo-path-in-fill.c \
    ../cairo-path-fixed.c \
    ../cairo-path-fill.c \
    ../cairo-path-bounds.c \
    ../cairo-paginated-surface.c \
    ../cairo-output-stream.c \
    ../cairo-observer.c \
    ../cairo-no-compositor.c \
    ../cairo-mutex.c \
    ../cairo-mono-scan-converter.c \
    ../cairo-misc.c \
    ../cairo-mesh-pattern-rasterizer.c \
    ../cairo-mempool.c \
    ../cairo-matrix.c \
    ../cairo-mask-compositor.c \
    ../cairo-lzw.c \
    ../cairo-image-surface.c \
    ../cairo-image-source.c \
    ../cairo-image-info.c \
    ../cairo-image-compositor.c \
    ../cairo-hull.c \
    ../cairo-hash.c \
    ../cairo-gstate.c \
    ../cairo-freelist.c \
    ../cairo-freed-pool.c \
    ../cairo-font-options.c \
    ../cairo-font-face.c \
    ../cairo-font-face-twin.c \
    ../cairo-font-face-twin-data.c \
    ../cairo-fixed.c \
    ../cairo-fallback-compositor.c \
    ../cairo-error.c \
    ../cairo-device.c \
    ../cairo-deflate-stream.c \
    ../cairo-default-context.c \
    ../cairo-debug.c \
    ../cairo-damage.c \
    ../cairo-contour.c \
    ../cairo-compositor.c \
    ../cairo-composite-rectangles.c \
    ../cairo-color.c \
    ../cairo-clip.c \
    ../cairo-clip-tor-scan-converter.c \
    ../cairo-clip-surface.c \
    ../cairo-clip-region.c \
    ../cairo-clip-polygon.c \
    ../cairo-clip-boxes.c \
    ../cairo-cff-subset.c \
    ../cairo-cache.c \
    ../cairo-boxes.c \
    ../cairo-boxes-intersect.c \
    ../cairo-botor-scan-converter.c \
    ../cairo-bentley-ottmann.c \
    ../cairo-bentley-ottmann-rectilinear.c \
    ../cairo-bentley-ottmann-rectangular.c \
    ../cairo-base85-stream.c \
    ../cairo-base64-stream.c \
    ../cairo-atomic.c \
    ../cairo-array.c \
    ../cairo-arc.c \
    ../cairo-analysis-surface.c

HEADERS += \
    ../../cairo.h \
    ../../cairo-version.h \
    ../../cairo-deprecated.h \
    ../../cairo-features.h \
    ../cairoint.h \
    ../cairo-wideint-type-private.h \
    ../cairo-wideint-private.h \
    ../cairo-user-font-private.h \
    ../cairo-types-private.h \
    ../cairo-type3-glyph-surface-private.h \
    ../cairo-tristrip-private.h \
    ../cairo-traps-private.h \
    ../cairo-time-private.h \
    ../cairo-surface-wrapper-private.h \
    ../cairo-surface-subsurface-private.h \
    ../cairo-surface-subsurface-inline.h \
    ../cairo-surface-snapshot-private.h \
    ../cairo-surface-snapshot-inline.h \
    ../cairo-surface-private.h \
    ../cairo-surface-offset-private.h \
    ../cairo-surface-observer-private.h \
    ../cairo-surface-observer-inline.h \
    ../cairo-surface-inline.h \
    ../cairo-surface-fallback-private.h \
    ../cairo-surface-clipper-private.h \
    ../cairo-surface-backend-private.h \
    ../cairo-stroke-dash-private.h \
    ../cairo-spans-private.h \
    ../cairo-spans-compositor-private.h \
    ../cairo-slope-private.h \
    ../cairo-scaled-font-subsets-private.h \
    ../cairo-scaled-font-private.h \
    ../cairo-rtree-private.h \
    ../cairo-region-private.h \
    ../cairo-reference-count-private.h \
    ../cairo-recording-surface-private.h \
    ../cairo-recording-surface-inline.h \
    ../cairo-private.h \
    ../cairo-pixman-private.h \
    ../cairo-pattern-private.h \
    ../cairo-pattern-inline.h \
    ../cairo-path-private.h \
    ../cairo-path-fixed-private.h \
    ../cairo-paginated-surface-private.h \
    ../cairo-paginated-private.h \
    ../cairo-output-stream-private.h \
    ../cairo-mutex-type-private.h \
    ../cairo-mutex-private.h \
    ../cairo-mutex-list-private.h \
    ../cairo-mutex-impl-private.h \
    ../cairo-mempool-private.h \
    ../cairo-malloc-private.h \
    ../cairo-list-private.h \
    ../cairo-list-inline.h \
    ../cairo-image-surface-private.h \
    ../cairo-image-surface-inline.h \
    ../cairo-image-info-private.h \
    ../cairo-hash-private.h \
    ../cairo-gstate-private.h \
    ../cairo-freelist-type-private.h \
    ../cairo-freelist-private.h \
    ../cairo-freed-pool-private.h \
    ../cairo-fontconfig-private.h \
    ../cairo-fixed-type-private.h \
    ../cairo-fixed-private.h \
    ../cairo-error-private.h \
    ../cairo-error-inline.h \
    ../cairo-device-private.h \
    ../cairo-default-context-private.h \
    ../cairo-damage-private.h \
    ../cairo-contour-private.h \
    ../cairo-contour-inline.h \
    ../cairo-compositor-private.h \
    ../cairo-composite-rectangles-private.h \
    ../cairo-compiler-private.h \
    ../cairo-combsort-inline.h \
    ../cairo-clip-private.h \
    ../cairo-clip-inline.h \
    ../cairo-cache-private.h \
    ../cairo-boxes-private.h \
    ../cairo-box-inline.h \
    ../cairo-backend-private.h \
    ../cairo-atomic-private.h \
    ../cairo-array-private.h \
    ../cairo-arc-private.h \
    ../cairo-analysis-surface-private.h \
    ../config.h \
    ../cairo-tee.h \
    ../cairo-tee-surface-private.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

OTHER_FILES += \
    ../cairo-uninstalled.pc.in \
    ../cairo-features-uninstalled.pc.in
