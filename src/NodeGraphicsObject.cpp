#include "NodeGraphicsObject.hpp"

#include <iostream>
#include <cstdlib>

#include <QtWidgets/QtWidgets>
#include <QtWidgets/QGraphicsEffect>

#include "ConnectionGraphicsObject.hpp"
#include "ConnectionState.hpp"

#include "FlowScene.hpp"
#include "NodePainter.hpp"

#include "Node.hpp"

NodeGraphicsObject::
NodeGraphicsObject(Node& node,
                   NodeState& nodeState,
                   NodeGeometry& nodeGeometry)
  : _node(node)
  , _nodeState(nodeState)
  , _nodeGeometry(nodeGeometry)
{
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsFocusable, true);

  setCacheMode( QGraphicsItem::DeviceCoordinateCache );

  {
    auto effect = new QGraphicsDropShadowEffect;
    effect->setOffset(4, 4);
    effect->setBlurRadius(20);
    effect->setColor(QColor(Qt::gray).darker(800));

    setGraphicsEffect(effect);

    setOpacity(_nodeGeometry.opacity());
  }

  FlowScene &flowScene = FlowScene::instance();
  flowScene.addItem(this);

  setAcceptHoverEvents(true);

  //embedQWidget();

  //_nodeGeometry.recalculateSize();
}


Node&
NodeGraphicsObject::
node()
{
  return _node;
}


void
NodeGraphicsObject::
embedQWidget()
{
  QPushButton* button = new QPushButton(QString("Hello"));

  QGraphicsProxyWidget* proxyWidget = new QGraphicsProxyWidget();

  proxyWidget->setWidget(button);

  button->setVisible(true);
  proxyWidget->setParentItem(this);
}


QRectF
NodeGraphicsObject::
boundingRect() const
{
  return _nodeGeometry.boundingRect();
}


void
NodeGraphicsObject::
paint(QPainter * painter,
      QStyleOptionGraphicsItem const* option,
      QWidget* )
{
  painter->setClipRect(option->exposedRect);

  NodePainter::paint(painter, _node);
}


void
NodeGraphicsObject::
mousePressEvent(QGraphicsSceneMouseEvent * event)
{
  auto clickEnd =
    [&](EndType endToCheck)
    {
      int portNumber = _nodeGeometry.checkHitScenePoint(endToCheck,
                                                        event->scenePos(),
                                                        _nodeState,
                                                        sceneTransform());

      FlowScene &flowScene = FlowScene::instance();

      if (portNumber != INVALID)
      {
        QUuid const connectionId = _nodeState.connectionID(endToCheck, portNumber);

        /// initialize new Connection
        if (connectionId.isNull())
        {
          // todo add to FlowScene
          auto connection = flowScene.createConnection();

          connection->connectionState().setDraggingEnd(endToCheck);

          _node.connect(connection.get(), portNumber);

          auto address = std::make_pair(_node.id(), portNumber);

          connection->setDraggingEnd(endToCheck);
          connection->connectToNode(address);

          connection->setDraggingEnd(oppositeEnd(endToCheck));
        }
        else
        {
          auto connection = flowScene.getConnection(connectionId);

          _node.disconnect(connection.get(), endToCheck, portNumber);

          connection->setDraggingEnd(endToCheck);
        }
      }
    };

  clickEnd(EndType::SINK);
  clickEnd(EndType::SOURCE);

  event->accept();
}


void
NodeGraphicsObject::
mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
  QPointF d = event->pos() - event->lastPos();

  if (event->lastPos() != event->pos())
    emit itemMoved(_node.id(), d);

  QGraphicsObject::mouseMoveEvent(event);
}


void
NodeGraphicsObject::
hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
  _nodeGeometry.setHovered(true);
  update();
  event->accept();
}


void
NodeGraphicsObject::
hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
  _nodeGeometry.setHovered(false);
  update();
  event->accept();
}