/****************************************************************************
**
* *Copyright (C) 2014
**
* *This file is generated by the Magus toolkit
**
* *THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* *"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* *LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* *A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* *OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* *SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* *LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* *DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* *THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* *(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* *OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "node_node.h"

#include <kamikaze/nodes.h>

#include <memory>

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTextCursor>

#include "node_constants.h"
#include "node_compound.h"
#include "node_port.h"
#include "node_editorwidget.h"

static constexpr auto NODE_HEADER_TITLE_FONT_SIZE = 12;
static constexpr auto NODE_HEADER_ICON_SIZE = 20.0f;
static constexpr auto NODE_WIDTH = 200.0f;
static constexpr auto NODE_HEADER_HEIGHT = 30.0f;
static constexpr auto NODE_BODY_HEIGHT = 32.0f;

static constexpr auto NODE_PEN_WIDTH_UNSELECTED = 1;
static constexpr auto NODE_PEN_WIDTH_SELECTED = 1;

static constexpr auto SELECTED_COLOR = "#52b1ee";  /* Light blue. */
//static constexpr auto SELECTED_COLOR = "#cc7800";  /* Orange. */

/* ************************************************************************** */

TextItem::TextItem(QGraphicsItem *parent)
    : TextItem("", parent)
{}

TextItem::TextItem(const QString &text, QGraphicsItem *parent)
    : QGraphicsTextItem(text, parent)
{
	this->setFlags(ItemIsSelectable | ItemIsFocusable);

	/* Switch off editor mode. */
	this->setTextInteractionFlags(Qt::NoTextInteraction);
}

void TextItem::setTextInteraction(bool on, bool select_all)
{
	if (on && textInteractionFlags() == Qt::NoTextInteraction) {
		/* Switch on editor mode. */
		this->setTextInteractionFlags(Qt::TextEditorInteraction);

		/* Manually do what a mouse click would do otherwise. */

		/* Give the item keyboard focus. */
		this->setFocus(Qt::MouseFocusReason);

		/* Ensure that itemChange() is called when we click out of the item. */
		this->setSelected(true);

		/* Select the whole text (e.g. after creation of the TextItem). */
		if (select_all) {
			QTextCursor c = textCursor();
			c.select(QTextCursor::Document);
			this->setTextCursor(c);
		}
	}
	else if (!on && textInteractionFlags() == Qt::TextEditorInteraction) {
		/* Switch off editor mode. */
		this->setTextInteractionFlags(Qt::NoTextInteraction);

		/* Deselect text (else it keeps gray shade). */
		QTextCursor c = this->textCursor();
		c.clearSelection();
		this->setTextCursor(c);
		this->clearFocus();

		/* Update underlying node. */

		/* TODO: find a better way to handle notification. */

		if (is_object_node(this->parentItem())) {
			auto node_item = static_cast<ObjectNodeItem *>(this->parentItem());
			node_item->scene_node()->name(this->toPlainText().toStdString());
			node_item->notifyEditor();
		}
		else {
			auto node_item = static_cast<QtNode *>(this->parentItem());
			node_item->getNode()->name(this->toPlainText().toStdString());
			node_item->notifyEditor();
		}
	}
}

void TextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (textInteractionFlags() == Qt::TextEditorInteraction) {
		/* If editor mode is already on, pass events onto the editor. */
		QGraphicsTextItem::mouseDoubleClickEvent(event);
		return;
	}

	/* If editor mode is off:
	 * 1. turn editor mode on and set selected and focused. */
	this->setTextInteraction(true);

	/* 2. send a single click to this QGraphicsTextItem (this will set the
	 * cursor to the mouse position): create a new mouse event with the same
	 * parameters as event. */
	auto click = std::make_unique<QGraphicsSceneMouseEvent>(QEvent::GraphicsSceneMousePress);
	click->setButton(event->button());
	click->setPos(event->pos());

	QGraphicsTextItem::mousePressEvent(click.get());
}

QVariant TextItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	if (   (change == QGraphicsItem::ItemSelectedChange)
	       && (textInteractionFlags() != Qt::NoTextInteraction)
	       && (!value.toBool()))
	{
		// item received SelectedChange event AND is in editor mode AND is about to be deselected:
		setTextInteraction(false); // leave editor mode
	}

	return QGraphicsTextItem::itemChange(change, value);
}

/* ************************************************************************** */

QtNode::QtNode(const QString &title, QGraphicsItem *parent)
    : QGraphicsPathItem(parent)
    , m_data(nullptr)
    , m_auto_size(true)
    , m_icon_size(NODE_HEADER_ICON_SIZE)
    , m_normalized_width(NODE_WIDTH)
    , m_width(m_normalized_width)
    , m_normalized_body_height(NODE_BODY_HEIGHT)
    , m_header_height(NODE_HEADER_HEIGHT)
    , m_body_height(m_normalized_body_height)
    , m_body(new QGraphicsPathItem(this))  /* MUST be a child of QtNode */
    , m_title_label(new TextItem(title, this))
    , m_title_alignment(ALIGNED_CENTER)
    , m_port_name_color(Qt::white)
    , m_header_title_icon(new QGraphicsPixmapItem(this))
    , m_active_connection(nullptr)
{
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);

	/* Header */
	m_header_brush = QBrush(QColor("#1f1f1f"));

	/* Icon */
	m_header_title_icon->setScale(0.0f);
	m_header_title_icon->setVisible(false);
	m_header_title_icon->setData(NODE_KEY_GRAPHIC_ITEM_TYPE, QVariant(NODE_VALUE_TYPE_HEADER_ICON));

	/* Body */
	m_body->setBrush(QColor("#3e3e3e"));
	m_body->setFlag(QGraphicsItem::ItemStacksBehindParent);

	/* Generic */
	setData(NODE_KEY_GRAPHIC_ITEM_TYPE, QVariant(NODE_VALUE_TYPE_NODE));
	m_body->setData(NODE_KEY_GRAPHIC_ITEM_TYPE, QVariant(NODE_VALUE_TYPE_NODE_BODY));

	/* Set title */
	m_title_label->setData(NODE_KEY_GRAPHIC_ITEM_TYPE, QVariant(NODE_VALUE_TYPE_HEADER_TITLE));
	m_font_header.setPointSize(NODE_HEADER_TITLE_FONT_SIZE);
	m_title_label->setFont(m_font_header);

	adjustWidthForTitle();
}

QtNode::~QtNode()
{
	deleteAllConnections();
}

/* Adjust the width of the node, depending on total width of the title and the
 * icons. Calculate using zoom factor 1.0 */
void QtNode::adjustWidthForTitle()
{
	m_font_header.setPointSize(NODE_HEADER_TITLE_FONT_SIZE);
	m_title_label->setFont(m_font_header);
	auto offset = 0.5f * (NODE_HEADER_HEIGHT - NODE_HEADER_ICON_SIZE);
	auto spaceLeft = m_normalized_width;

	if (m_header_title_icon->isVisible()) {
        spaceLeft -= NODE_HEADER_ICON_SIZE;
        spaceLeft -= offset;
    }

	if (m_title_label->boundingRect().width() > spaceLeft) {
		m_normalized_width += m_title_label->boundingRect().width() - spaceLeft;
	}

	/* Set values to original zoom factor */
	m_font_header.setPointSize(NODE_HEADER_TITLE_FONT_SIZE);
	m_title_label->setFont(m_font_header);
	m_width = m_normalized_width;

	redraw();
}

void QtNode::redraw()
{
	/* Redraw the node */
	const auto halfWidth = 0.5f * m_width;
	const auto offset = 0.5f * (NODE_HEADER_HEIGHT - NODE_HEADER_ICON_SIZE);

	auto p = QPainterPath{};
	p.addRoundedRect(-halfWidth, 0, m_width, m_header_height, 4, 5);
	setPath(p);

	auto bodyPath = QPainterPath{};
	bodyPath.addRoundedRect(-halfWidth, 0, m_width, m_body_height, 4, 5);
	m_body->setPath(bodyPath);

	m_font_header.setPointSize(NODE_HEADER_TITLE_FONT_SIZE);
	m_title_label->setFont(m_font_header);

	if (m_header_title_icon->isVisible()) {
		m_header_title_icon->setScale(m_icon_size / m_header_title_icon->pixmap().width()); // Scale may only be set if visible
	}

	m_header_title_icon->setPos(-halfWidth + offset, offset);

	setTitlePosition();

	/* Redraw image */
	auto height = NODE_BODY_HEIGHT;

	/* Redraw ports */
	for (auto port : m_port_list) {
		port->redraw();

		if (port->isVisible()) {
			/* This is in case the node is not collapsed */
			height += NODE_PORT_HEIGHT_MARGIN_FACTOR * port->getNormalizedHeight();
			setPortAlignedPos(port, height);
		}
		else {
			/* This is in case the node (and its ports) is collapsed; the port's
			 * height must be on the correct y-position (on the header), so the
			 * connections also remain on that position. */
			height = NODE_PORT_HEIGHT_MARGIN_FACTOR * port->getNormalizedHeight();
			setPortAlignedPos(port, height);
		}
	}
}

void QtNode::setTitleColor(const QColor &color)
{
	m_title_label->setDefaultTextColor(color);
}

void QtNode::alignTitle(Alignment alignment)
{
	m_title_alignment = alignment;
	setTitlePosition();
}

void QtNode::setTitlePosition()
{
	const auto halfWidth = 0.5f * m_width;
	const auto offset = 0.5f * (m_header_height - m_icon_size);

	switch (m_title_alignment) {
		case ALIGNED_LEFT:
		{
			if (m_header_title_icon->isVisible()) {
                // Position it right to the icon
                m_title_label->setPos(-halfWidth + m_icon_size + offset,
                                    0.5 * (m_header_height - m_title_label->boundingRect().height()));
            }
			else {
				/* Position it where the icon should be */
				m_title_label->setPos(-halfWidth + offset,
				                      0.5 * (m_header_height - m_title_label->boundingRect().height()));
			}

			break;
		}
		case ALIGNED_RIGHT:
		{
			/* Position it right */
			m_title_label->setPos(halfWidth - offset - m_title_label->boundingRect().width(),
			                      0.5 * (m_header_height - m_title_label->boundingRect().height()));

			break;
		}
		case ALIGNED_CENTER:
		{
			m_title_label->setPos(-0.5 * m_title_label->boundingRect().width(),
			                      0.5 * (m_header_height - m_title_label->boundingRect().height()));
			break;
		}
	}
}

void QtNode::setEditor(QtNodeEditor *editor)
{
	m_editor = editor;
}

void QtNode::setScene(QGraphicsScene *scene)
{
	m_scene = scene;
}

bool QtNode::mouseLeftClickHandler(QGraphicsSceneMouseEvent *mouseEvent,
                                   QGraphicsItem *item,
                                   unsigned int action,
                                   QtConnection *activeConnection)
{
	auto type = 0;

	if (item->data(NODE_KEY_GRAPHIC_ITEM_TYPE).isValid()) {
		type = item->data(NODE_KEY_GRAPHIC_ITEM_TYPE).toInt();
	}

	switch (type) {
		case NODE_VALUE_TYPE_PORT:
		{
			auto port = static_cast<QtPort *>(item);

			/* Check wether the port is available; if not, return false. Only
			 * applies to input ports for now. */
			if (!port->isPortOpen() && !port->isOutputPort()) {
				return false;
			}

			if (action == NODE_ACTION_BASE) {
				/* Create a new connection; this node is the baseNode (with the base port) */
				createActiveConnection(port, mouseEvent->scenePos());
			}
			else if (action == NODE_ACTION_TARGET && activeConnection) {
				/* Check wether the connection is allowed; if not, return false */
				/* This node is the target node (with the target port) */
				auto basePort = activeConnection->getBasePort();

				if (!is_connection_allowed(port, basePort)) {
					return false;
				}

				/* Finalize the connection; this node is the targetNode */
				if (activeConnection) {
					port->createConnection(activeConnection);

					/* used to be Q_EMIT, but does not work for some reason */
					m_editor->connectionEstablished(activeConnection);
				}
			}

			break;
		}
		default:
			break;
	}

	return true;
}

void QtNode::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	this->getNode()->xpos(this->scenePos().x());
	this->getNode()->ypos(this->scenePos().y());
	return QGraphicsPathItem::mouseMoveEvent(event);
}

void QtNode::collapse()
{
	/* Set visibility of the body */
	m_body->setVisible(false);

	/* Set visibility of the ports */
	for (auto port : m_port_list) {
		if (port->isVisible()) {
			port->collapse();
		}
	}
}

void QtNode::expand()
{
	/* Set visibility of the body */
	m_body->setVisible(true);

	/* Set visibility of the ports */
	for (auto port : m_port_list) {
		if (!port->isVisible()) {
			port->expand();
		}
	}
}

bool QtNode::hasInputs() const
{
	return !m_input_ports.empty();
}

bool QtNode::hasOutputs() const
{
	return !m_output_ports.empty();
}

QtPort *QtNode::input(int index) const
{
	return m_input_ports[index];
}

QtPort *QtNode::input(const QString &name) const
{
	for (QtPort *port : m_input_ports) {
		if (port->getPortName() == name) {
			return port;
		}
	}

	return nullptr;
}

QtPort *QtNode::output(int index) const
{
	return m_output_ports[index];
}

QtPort *QtNode::output(const QString &name) const
{
	for (QtPort *port : m_output_ports) {
		if (port->getPortName() == name) {
			return port;
		}
	}

	return nullptr;
}

void QtNode::createActiveConnection(QtPort *port, QPointF pos)
{
	m_active_connection = port->createConnection();
	m_scene->addItem(m_active_connection);
	m_active_connection->updatePath(pos);
}

void QtNode::deleteActiveConnection()
{
	if (m_active_connection) {
		auto port = m_active_connection->getBasePort();
		m_scene->removeItem(m_active_connection);
		port->deleteConnection(m_active_connection);
		m_active_connection = nullptr;
	}
}

void QtNode::deleteAllConnections()
{
	for (auto port : m_port_list) {
		for (auto connection : port->getConnections()) {
			m_scene->removeItem(connection);
		}

		port->deleteAllConnections();
	}
}

void QtNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	/* Set header color. */
	if (m_data && m_data->has_warning()) {
		m_header_brush.setColor(QColor("#ffd42a"));
	}
	else {
		m_header_brush.setColor(QColor("#1f1f1f"));
	}

	/* Paint header */
	painter->setBackgroundMode(Qt::OpaqueMode);
	painter->setBrush(m_header_brush);

	if (isSelected()) {
		m_pen.setColor(SELECTED_COLOR);
		m_pen.setWidth(NODE_PEN_WIDTH_SELECTED);
	}
	else {
		m_pen.setColor(Qt::black);
		m_pen.setWidth(NODE_PEN_WIDTH_UNSELECTED);
	}

	painter->setPen(m_pen);
	m_body->setPen(m_pen);
	painter->drawPath(path());

	/* Only update the connection if it is not the active one. */
	for (auto port : m_port_list) {
		for (auto connection : port->getConnections()) {
			if (connection != m_active_connection) {
				connection->updatePath();
			}
		}
	}
}

void QtNode::setNode(Node *node)
{
	m_data = node;

	for (const auto &input : node->inputs()) {
		createPort(input->name.c_str(),
		           NODE_PORT_TYPE_INPUT,
		           QColor(95, 95, 95),
		           ALIGNED_LEFT,
		           QColor(95, 95, 95));
	}

	for (const auto &output : node->outputs()) {
		createPort(output->name.c_str(),
		           NODE_PORT_TYPE_OUTPUT,
		           QColor(95, 95, 95),
		           ALIGNED_RIGHT,
		           QColor(95, 95, 95));
	}

	/* Set the icon. */
	const auto &path = node->icon_path();

	if (path.empty()) {
		return;
	}

	QPixmap pixmap(path.c_str());

	/* Sanity check. */
    if (pixmap.width() != 0) {
		m_header_title_icon->setPixmap(pixmap);
        m_header_title_icon->setScale(m_icon_size / pixmap.width());
        m_header_title_icon->setVisible(true);
		adjustWidthForTitle();
    }
}

Node *QtNode::getNode() const
{
	return m_data;
}

QtPort *QtNode::createPort(const QString &portName,
                           int type,
                           QColor portColour,
                           Alignment alignement,
                           QColor connectionColor)
{
	auto port = new QtPort(portName, type, portColour, connectionColor, alignement, this);
	port->setNameColor(m_port_name_color);
	port->setData(NODE_KEY_GRAPHIC_ITEM_TYPE, QVariant(NODE_VALUE_TYPE_PORT));
	m_port_list.append(port);

	if (type == NODE_PORT_TYPE_INPUT) {
		m_input_ports.push_back(port);
	}
	else {
		m_output_ports.push_back(port);
	}

	if (m_auto_size) {
		m_normalized_body_height += NODE_PORT_HEIGHT_MARGIN_FACTOR * port->getNormalizedHeight();
		m_body_height = m_normalized_body_height;

		/* Adjust width of the node (if needed) to fit the port */
		auto normalized = port->getNormalizedWidth() + 2.5f * NODE_PORT_OFFSET;
		if (normalized > m_normalized_width)
			m_normalized_width = normalized;
		m_width = m_normalized_width;
	}

	setPortAlignedPos(port, m_body_height);
	redraw();

	return port;
}

void QtNode::setPortAlignedPos(QtPort *port, qreal height)
{
	/* Position */
	auto halfWidth = 0.5f * m_width;
	auto offset = NODE_PORT_OFFSET;
	auto portWidth = port->getNormalizedWidth();

	switch (port->getAlignment()) {
		case ALIGNED_LEFT:
			port->setAlignedPos(-halfWidth + offset, height - offset);
			break;
		case ALIGNED_RIGHT:
			port->setAlignedPos(halfWidth - offset, height - offset);
			break;
		case ALIGNED_CENTER:
			port->setAlignedPos(-0.5 * portWidth, height - offset);
			break;
	}
}

void QtNode::setParentItem(QGraphicsItem *parent)
{
	m_original_parent = parentItem();
	QGraphicsPathItem::setParentItem(parent);
}

void QtNode::restoreOriginalParentItem()
{
	setParentItem(m_original_parent);
}

bool QtNode::isConnectionConnectedToThisNode(QtConnection *connection)
{
	for (auto port : m_port_list) {
		for (auto co : port->getConnections()) {
			if (co == connection) {
				return true;
			}
		}
	}

	return false;

#if 0
	if (connection->getBasePort()) {
		if (!(connection->getBasePort()->parentItem())) {
			return false;
		}

		auto base = static_cast<QtNode *>(connection->getBasePort()->parentItem());
		return (base == this);
	}

	if (connection->getTargetPort()) {
		if (!(connection->getTargetPort()->parentItem())) {
			return false;
		}

		auto target = static_cast<QtNode *>(connection->getTargetPort()->parentItem());
		return (target == this);
	}

	return false;
#endif
}

void QtNode::notifyEditor() const
{
	m_editor->sendNotification();
}
