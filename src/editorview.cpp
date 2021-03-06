// This file is part of Heimer.
// Copyright (C) 2018 Jussi Lind <jussi.lind@iki.fi>
//
// Heimer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Heimer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Heimer. If not, see <http://www.gnu.org/licenses/>.

#include <QApplication>
#include <QColorDialog>
#include <QGraphicsItem>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <QStatusBar>
#include <QString>
#include <QTransform>

#include "editorview.hpp"

#include "draganddropstore.hpp"
#include "edge.hpp"
#include "graphicsfactory.hpp"
#include "mclogger.hh"
#include "mediator.hpp"
#include "mindmapdata.hpp"
#include "node.hpp"
#include "nodehandle.hpp"

#include <cassert>
#include <cstdlib>

EditorView::EditorView(Mediator & mediator)
    : m_mediator(mediator)
{
    createBackgroundContextMenuActions();
    createNodeContextMenuActions();

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);

    setRenderHint(QPainter::Antialiasing);
}

void EditorView::createBackgroundContextMenuActions()
{
    auto setColorAction = new QAction(QWidget::tr("Set background color"), &m_backgroundContextMenu);
    QObject::connect(setColorAction, &QAction::triggered, [this] () {
        const auto color = QColorDialog::getColor(Qt::white, this);
        if (color.isValid()) {
            emit backgroundColorChanged(color);
        }
    });

    auto createNode = new QAction(QWidget::tr("Create floating node"), &m_backgroundContextMenu);
    QObject::connect(createNode, &QAction::triggered, [this] () {
        emit newNodeRequested(m_clickedScenePos);
    });

    // Populate the menu
    m_backgroundContextMenu.addAction(setColorAction);
    m_backgroundContextMenu.addSeparator();
    m_backgroundContextMenu.addAction(createNode);
}

void EditorView::createNodeContextMenuActions()
{
    m_setNodeColorAction = new QAction(tr("Set node color"), &m_nodeContextMenu);
    QObject::connect(m_setNodeColorAction, &QAction::triggered, [this] () {
        assert(m_mediator.selectedNode());
        const auto color = QColorDialog::getColor(Qt::white, this);
        if (color.isValid()) {
            m_mediator.saveUndoPoint();
            m_mediator.selectedNode()->setColor(color);
        }
    });

    m_deleteNodeAction = new QAction(tr("Delete node"), &m_nodeContextMenu);
    QObject::connect(m_deleteNodeAction, &QAction::triggered, [this] () {
        assert(m_mediator.selectedNode());
        m_mediator.saveUndoPoint();
        m_mediator.deleteNode(*m_mediator.selectedNode());
    });

    // Populate the menu
    m_nodeContextMenu.addAction(m_setNodeColorAction);
    m_nodeContextMenu.addSeparator();
    m_nodeContextMenu.addAction(m_deleteNodeAction);
}

void EditorView::handleMousePressEventOnBackground(QMouseEvent & event)
{
    if (!m_mediator.hasNodes())
    {
        return;
    }

    if (event.button() == Qt::LeftButton)
    {
        m_mediator.dadStore().setSourceNode(nullptr, DragAndDropStore::Action::Scroll);
        setDragMode(ScrollHandDrag);
    }
    else if (event.button() == Qt::RightButton)
    {
        openBackgroundContextMenu();
    }
}

void EditorView::handleMousePressEventOnNode(QMouseEvent & event, Node & node)
{
    if (event.button() == Qt::RightButton)
    {
        handleRightButtonClickOnNode(node);
    }
    else if (event.button() == Qt::LeftButton)
    {
        handleLeftButtonClickOnNode(node);
    }
}

void EditorView::handleMousePressEventOnNodeHandle(QMouseEvent & event, NodeHandle & nodeHandle)
{
    if (event.button() == Qt::LeftButton)
    {
        handleLeftButtonClickOnNodeHandle(nodeHandle);
    }
}

void EditorView::handleLeftButtonClickOnNode(Node & node)
{
    // User is initiating a node move drag

    m_mediator.saveUndoPoint();

    node.setZValue(node.zValue() + 1);
    m_mediator.dadStore().setSourceNode(&node, DragAndDropStore::Action::MoveNode);
    m_mediator.dadStore().setSourcePos(m_mappedPos - node.pos());

    // Change cursor to the closed hand cursor.
    QApplication::setOverrideCursor(QCursor(Qt::ClosedHandCursor));
}

void EditorView::handleLeftButtonClickOnNodeHandle(NodeHandle & nodeHandle)
{
    switch (nodeHandle.role())
    {
    case NodeHandle::Role::Add:
        initiateNewNodeDrag(nodeHandle);
        break;
    case NodeHandle::Role::Color:
        m_mediator.setSelectedNode(&nodeHandle.parentNode());
        m_setNodeColorAction->trigger();
        break;
    default:
        break;
    }
}

void EditorView::handleRightButtonClickOnNode(Node & node)
{
    m_mediator.setSelectedNode(&node);

    openNodeContextMenu();
}

void EditorView::initiateNewNodeDrag(NodeHandle & nodeHandle)
{
    // User is initiating a new node drag
    m_mediator.saveUndoPoint();
    auto parentNode = dynamic_cast<Node *>(nodeHandle.parentItem());
    assert(parentNode);
    m_mediator.dadStore().setSourceNode(parentNode, DragAndDropStore::Action::CreateNode);
    m_mediator.dadStore().setSourcePos(nodeHandle.pos());
    // Change cursor to the closed hand cursor.
    QApplication::setOverrideCursor(QCursor(Qt::ClosedHandCursor));
}

void EditorView::mouseMoveEvent(QMouseEvent * event)
{
    m_mappedPos = mapToScene(event->pos());

    switch (m_mediator.dadStore().action())
    {
    case DragAndDropStore::Action::MoveNode:
        if (auto node = m_mediator.dadStore().sourceNode())
        {
            node->setLocation(m_mappedPos - m_mediator.dadStore().sourcePos());
        }
        break;
    case DragAndDropStore::Action::CreateNode:
        showDummyDragNode(true);
        showDummyDragEdge(true);
        m_dummyDragNode->setPos(m_mappedPos - m_mediator.dadStore().sourcePos());
        m_dummyDragEdge->updateLine();
        m_mediator.dadStore().sourceNode()->setHandlesVisible(false);
        break;
    case DragAndDropStore::Action::Scroll:
        break;
    default:
        break;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void EditorView::mousePressEvent(QMouseEvent * event)
{
    m_clickedPos = event->pos();
    m_clickedScenePos = mapToScene(m_clickedPos);

    // Fetch all items at the location
    QList<QGraphicsItem *> items = scene()->items(
        m_clickedScenePos, Qt::IntersectsItemShape, Qt::DescendingOrder);

    if (items.size())
    {
        auto item = *items.begin();
        if (auto node = dynamic_cast<Node *>(item))
        {
            handleMousePressEventOnNode(*event, *node);
        }
        else if (auto node = dynamic_cast<NodeHandle *>(item))
        {
            handleMousePressEventOnNodeHandle(*event, *node);
        }
    }
    else
    {
        handleMousePressEventOnBackground(*event);
    }

    QGraphicsView::mousePressEvent(event);
}

void EditorView::mouseReleaseEvent(QMouseEvent * event)
{
    switch (m_mediator.dadStore().action())
    {
    case DragAndDropStore::Action::MoveNode:
        m_mediator.dadStore().clear();
        break;
    case DragAndDropStore::Action::CreateNode:
        if (auto sourceNode = m_mediator.dadStore().sourceNode())
        {
            m_mediator.createAndAddNode(sourceNode->index(), m_mappedPos - m_mediator.dadStore().sourcePos());

            resetDummyDragItems();

            m_mediator.dadStore().clear();
        }
        break;
    case DragAndDropStore::Action::Scroll:
        setDragMode(NoDrag);
        break;
    default:
        break;
    }

    QApplication::restoreOverrideCursor();

    QGraphicsView::mouseReleaseEvent(event);
}

void EditorView::openBackgroundContextMenu()
{
    m_backgroundContextMenu.exec(mapToGlobal(m_clickedPos));
}

void EditorView::openNodeContextMenu()
{
    m_deleteNodeAction->setEnabled(m_mediator.isLeafNode(*m_mediator.selectedNode()) || m_mediator.isInBetween(*m_mediator.selectedNode()));

    m_nodeContextMenu.exec(mapToGlobal(m_clickedPos));
}

void EditorView::resetDummyDragItems()
{
    // Ensure new dummy nodes and related graphics items are created (again) when needed.
    delete m_dummyDragEdge;
    m_dummyDragEdge = nullptr;
    delete m_dummyDragNode;
    m_dummyDragNode = nullptr;

    MCLogger().debug() << "Dummy drag item reset";
}

void EditorView::showDummyDragEdge(bool show)
{
    if (auto sourceNode = m_mediator.dadStore().sourceNode())
    {
        if (!m_dummyDragEdge)
        {
            MCLogger().debug() << "Creating a new dummy drag edge";
            m_dummyDragEdge = new Edge(*sourceNode, *m_dummyDragNode);
            m_dummyDragEdge->setOpacity(0.5f);
            scene()->addItem(m_dummyDragEdge);
        }
        else
        {
            m_dummyDragEdge->setSourceNode(*sourceNode);
            m_dummyDragEdge->setTargetNode(*m_dummyDragNode);
        }
    }

    m_dummyDragEdge->setVisible(show);
}

void EditorView::showDummyDragNode(bool show)
{
    if (!m_dummyDragNode)
    {
        MCLogger().debug() << "Creating a new dummy drag node";
        m_dummyDragNode = new Node;
        scene()->addItem(m_dummyDragNode);
    }

    m_dummyDragNode->setOpacity(0.5f);
    m_dummyDragNode->setVisible(show);
}

void EditorView::updateScale(int value)
{
    qreal scale = static_cast<qreal>(value) / 100;

    QTransform transform;
    transform.scale(scale, scale);
    setTransform(transform);
}

void EditorView::wheelEvent(QWheelEvent * event)
{
    const int sensitivity = 10;
    zoom(event->delta() > 0 ? sensitivity : -sensitivity);
}

void EditorView::zoom(int amount)
{
    m_scaleValue += amount;
    m_scaleValue = std::min(m_scaleValue, 200);
    m_scaleValue = std::max(m_scaleValue, 10);

    updateScale(m_scaleValue);
}

void EditorView::zoomToFit(QRectF nodeBoundingRect)
{
    const float viewAspect = float(rect().height()) / rect().width();
    const float nodeAspect = float(nodeBoundingRect.height()) / nodeBoundingRect.width();

    if (viewAspect < 1.0)
    {
        if (nodeAspect < viewAspect)
        {
            m_scaleValue = rect().width() * 100 / nodeBoundingRect.width();
        }
        else
        {
            m_scaleValue = rect().height() * 100 / nodeBoundingRect.height();
        }
    }
    else
    {
        if (nodeAspect > viewAspect)
        {
            m_scaleValue = rect().height() * 100 / nodeBoundingRect.height();
        }
        else
        {
            m_scaleValue = rect().width() * 100 / nodeBoundingRect.width();
        }
    }

    updateScale(m_scaleValue);

    centerOn(nodeBoundingRect.center());
}

EditorView::~EditorView()
{
}
