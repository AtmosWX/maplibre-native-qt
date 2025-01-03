// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "gl_widget.hpp"
#include "gl_widget_p.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

namespace QMapLibre {

/*!
    \defgroup QMapLibreWidgets QMapLibre Widgets
    \brief Qt Widgets for MapLibre.
*/

/*!
    \class GLWidget
    \brief A simple OpenGL widget that displays a \ref QMapLibre::Map.
    \ingroup QMapLibreWidgets

    \headerfile gl_widget.hpp <QMapLibreWidgets/GLWidget>

    The widget is intended as a standalone map viewer in a Qt Widgets application.
    It owns its own instance of \ref QMapLibre::Map that is rendered to the widget.

    \fn GLWidget::onMouseDoubleClickEvent
    \brief Emitted when the user double-clicks the mouse.

    \fn GLWidget::onMouseMoveEvent
    \brief Emitted when the user moves the mouse.

    \fn GLWidget::onMousePressEvent
    \brief Emitted when the user presses the mouse.

    \fn GLWidget::onMouseReleaseEvent
    \brief Emitted when the user releases the mouse.
*/

/*! Default constructor */
GLWidget::GLWidget(const Settings &settings)
    : d_ptr(std::make_unique<GLWidgetPrivate>(this, settings)) {
    d_ptr->m_map = std::make_unique<Map>(nullptr, d_ptr->m_settings, size(), devicePixelRatioF());
}

GLWidget::~GLWidget() {
    // Make sure we have a valid context so we
    // can delete the Map.
    makeCurrent();
    d_ptr.reset();
}

/*!
    \brief Get the QMapLibre::Map instance.
*/
Map *GLWidget::map() {
    return d_ptr->m_map.get();
}

/*!
    \brief Mouse press event handler.
*/
void GLWidget::mousePressEvent(QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPointF &position = event->position();
#else
    const QPointF &position = event->localPos();
#endif
    emit onMousePressEvent(d_ptr->m_map->coordinateForPixel(position));
    if (event->type() == QEvent::MouseButtonDblClick) {
        emit onMouseDoubleClickEvent(d_ptr->m_map->coordinateForPixel(position));
    }

    d_ptr->handleMousePressEvent(event);
}

/*!
    \brief Mouse release event handler.
*/
void GLWidget::mouseReleaseEvent(QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPointF &position = event->position();
#else
    const QPointF &position = event->localPos();
#endif
    emit onMouseReleaseEvent(d_ptr->m_map->coordinateForPixel(position));
}

/*!
    \brief Mouse move event handler.
*/
void GLWidget::mouseMoveEvent(QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPointF &position = event->position();
#else
    const QPointF &position = event->localPos();
#endif
    emit onMouseMoveEvent(d_ptr->m_map->coordinateForPixel(position));

    d_ptr->handleMouseMoveEvent(event);
}

/*!
    \brief Mouse wheel event handler.
*/
void GLWidget::wheelEvent(QWheelEvent *event) {
    d_ptr->handleWheelEvent(event);
}

/*!
    \brief Initializes the map and sets up default settings.

    This function is called internally by Qt when the widget is initialized.
*/
void GLWidget::initializeGL() {
    initializeOpenGLFunctions();

    connect(d_ptr->m_map.get(), SIGNAL(needsRendering()), this, SLOT(update()));

    // Set default location
    d_ptr->m_map->setCoordinateZoom(d_ptr->m_settings.defaultCoordinate(), d_ptr->m_settings.defaultZoom());

    // Set default style
    if (!d_ptr->m_settings.styles().empty()) {
        d_ptr->m_map->setStyleUrl(d_ptr->m_settings.styles().front().url);
    } else if (!d_ptr->m_settings.providerStyles().empty()) {
        d_ptr->m_map->setStyleUrl(d_ptr->m_settings.providerStyles().front().url);
    }
}

/*!
    \brief Renders the map.

    This function is called internally by Qt when the widget needs to be redrawn.
*/
void GLWidget::paintGL() {
    d_ptr->m_map->resize(size());
    d_ptr->m_map->setOpenGLFramebufferObject(defaultFramebufferObject(), size() * devicePixelRatioF());
    glClearColor(0.0667f, 0.0667f, 0.1059f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    d_ptr->m_map->render();
}

/*! \cond PRIVATE */

// GLWidgetPrivate
GLWidgetPrivate::GLWidgetPrivate(QObject *parent, Settings settings)
    : QObject(parent),
      m_settings(std::move(settings)) {}

GLWidgetPrivate::~GLWidgetPrivate() = default;

void GLWidgetPrivate::handleMousePressEvent(QMouseEvent *event) {
    constexpr double zoomInScale{2.0};
    constexpr double zoomOutScale{0.5};

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_lastPos = event->position();
#else
    m_lastPos = event->localPos();
#endif

    if (event->type() == QEvent::MouseButtonDblClick) {
        if (event->buttons() == Qt::LeftButton) {
            m_map->scaleBy(zoomInScale, m_lastPos);
        } else if (event->buttons() == Qt::RightButton) {
            m_map->scaleBy(zoomOutScale, m_lastPos);
        }
    }

    event->accept();
}

void GLWidgetPrivate::handleMouseMoveEvent(QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPointF &position = event->position();
#else
    const QPointF &position = event->localPos();
#endif

    const QPointF delta = position - m_lastPos;
    if (!delta.isNull()) {
        if (event->buttons() == Qt::LeftButton) {
            m_map->moveBy(delta);
        } else if (event->buttons() == Qt::RightButton) {
            constexpr double rotationFactor = 0.5;
            constexpr double pitchFactor = 0.5;
            m_map->setBearing(m_map->bearing() + delta.x() * rotationFactor);
            m_map->pitchBy(delta.y() * pitchFactor);
        }
    }

    m_lastPos = position;
    event->accept();
}

void GLWidgetPrivate::handleWheelEvent(QWheelEvent *event) const {
    if (event->angleDelta().y() == 0) {
        return;
    }

    constexpr float wheelConstant = 240.f;

    float factor = static_cast<float>(event->angleDelta().y()) / wheelConstant;
    if (event->angleDelta().y() < 0) {
        factor = factor > -1 ? factor : 1 / factor;
    }

    constexpr auto smooth_zoom_easing = std::chrono::milliseconds(1000 / 5);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    m_map->easeTo({.anchor = event->position(), .zoom = m_map->zoom() + factor}, smooth_zoom_easing);
#else
    m_map->easeTo({.anchor = event->pos(), .zoom = m_map->zoom() + factor}, smooth_zoom_easing);
#endif
    event->accept();
}

/*! \endcond */

} // namespace QMapLibre
