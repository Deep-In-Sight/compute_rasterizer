#include <CameraController.h>
#include <PointCloudItem.h>
#include <PointCloudQuickRenderer.h>
#include <QDebug>
#include <QMimeData>
#include <QQuickWindow>

PointCloudItem::PointCloudItem(QQuickItem *parent) : QQuickFramebufferObject(parent)
{
    setFlag(ItemAcceptsDrops, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setAcceptTouchEvents(true);
    setMirrorVertically(true); // conform to opengl texture coordinates
    setTextureFollowsItemSize(true);
    static int itemCounter = 0;
    item_name = QString("PointCloudItem") + QString::number(itemCounter++);
    qDebug() << item_name << " Created";
}

void PointCloudItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickFramebufferObject::geometryChanged(newGeometry, oldGeometry);
    update();
}

void PointCloudItem::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void PointCloudItem::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void PointCloudItem::dropEvent(QDropEvent *event)
{
    auto urls = event->mimeData()->urls();
    if (urls.isEmpty())
    {
        return;
    }
    std::vector<std::string> lasPaths;
    for (const auto &url : urls)
    {
        auto path = url.toLocalFile();
        if (path.endsWith(".las"))
        {
            lasPaths.push_back(path.toStdString());
        }
    }
    if (!lasPaths.empty())
    {
        m_renderer->addLasFiles(lasPaths);
        glm::dvec3 center, size;
        m_renderer->getSceneBox(center, size);
        cameraController->setCenterView({center[0], center[1], center[2]});
    }
    qDebug() << item_name << " Dropped " << lasPaths.size() << " files";
}

CameraController::MouseButton buttonMap(Qt::MouseButton b)
{
    switch (b)
    {
    case Qt::LeftButton:
        return CameraController::MouseButton::LEFT;
    case Qt::RightButton:
        return CameraController::MouseButton::RIGHT;
    default:
        return CameraController::MouseButton::NONE;
    }
}

void PointCloudItem::mousePressEvent(QMouseEvent *event)
{
    CameraController::MouseButton button = buttonMap(event->button());
    cameraController->mousePressed(button, event->x(), event->y());
}

void PointCloudItem::mouseReleaseEvent(QMouseEvent *event)
{
    CameraController::MouseButton button = buttonMap(event->button());
    cameraController->mouseReleased(button);
}

void PointCloudItem::mouseMoveEvent(QMouseEvent *event)
{
    cameraController->mouseMoved(event->x(), event->y());
}

void PointCloudItem::wheelEvent(QWheelEvent *event)
{
    cameraController->mouseWheelRolled(event->angleDelta().y());
}

void PointCloudItem::touchEvent(QTouchEvent *event)
{
    for (const auto &touchPoint : event->touchPoints())
    {
        switch (touchPoint.state())
        {
        case Qt::TouchPointPressed:
            cameraController->touchPressed(touchPoint.id(), touchPoint.pos().x(), touchPoint.pos().y());
            break;
        case Qt::TouchPointReleased:
            cameraController->touchReleased(touchPoint.id());
            break;
        case Qt::TouchPointMoved: {
            auto moved = touchPoint.pos() - touchPoint.lastPos();
            cameraController->touchMoved(touchPoint.id(), moved.x(), moved.y());
            break;
        }
        default:
            break;
        }
    }
}

PointCloudItem::Renderer *PointCloudItem::createRenderer() const
{
    m_renderer = new PointCloudQuickRenderer();
    cameraController = m_renderer->getCameraController();
    return m_renderer;
}

bool PointCloudItem::event(QEvent *event)
{
    if (cameraController == nullptr)
    {
        return false;
    }
    bool handled = QQuickFramebufferObject::event(event);
    update();
    return handled;
}