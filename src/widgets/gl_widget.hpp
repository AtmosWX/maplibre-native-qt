// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#ifndef QMAPLIBRE_GL_WIDGET_H
#define QMAPLIBRE_GL_WIDGET_H

#include <QMapLibreWidgets/Export>

#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

#include <memory>

QT_BEGIN_NAMESPACE

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

QT_END_NAMESPACE

namespace QMapLibre {

class GLWidgetPrivate;

class Q_MAPLIBRE_WIDGETS_EXPORT GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GLWidget(const Settings &);
    ~GLWidget() override;

    Map *map();

signals:
    void onMouseDoubleClickEvent(QMapLibre::Coordinate coordinate);
    void onMousePressEvent(QMapLibre::Coordinate coordinate);
    void onMouseReleaseEvent(QMapLibre::Coordinate coordinate);
    void onMouseMoveEvent(QMapLibre::Coordinate coordinate);

protected:
    // QWidget implementation.
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    // Q{,Open}GLWidget implementation.
    void initializeGL() override;
    void paintGL() override;

private:
    Q_DISABLE_COPY(GLWidget)

    std::unique_ptr<GLWidgetPrivate> d_ptr;
};

} // namespace QMapLibre

#endif // QMAPLIBRE_GL_WIDGET_H
