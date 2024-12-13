#pragma once

#include <QQuickFramebufferObject>

struct PointCloudQuickRenderer;
struct CameraController;

class PointCloudItem : public QQuickFramebufferObject
{
    Q_OBJECT
  public:
    PointCloudItem(QQuickItem *parent = nullptr);
    ~PointCloudItem() = default;

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

  protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void touchEvent(QTouchEvent *event) override;
    bool event(QEvent *event) override;

    QQuickFramebufferObject::Renderer *createRenderer() const override;

  private:
    mutable PointCloudQuickRenderer *m_renderer = nullptr;
    mutable CameraController *cameraController = nullptr;
};