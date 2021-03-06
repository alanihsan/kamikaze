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

#pragma once

#include <QGraphicsPathItem>

#include "node_constants.h"

class QtNode;
class QtPort;

/* ***************************************************************************
 * QtConnection establishes a connection between two ports of different nodes.
 * Depending on the allowed porttypes, this connection may or may not be
 * established.
 ***************************************************************************/
class QtConnection : public QGraphicsPathItem {
	QtPort *m_base_port = nullptr;
	QtPort *m_target_port = nullptr;
	QColor m_color = Qt::black;

public:
	explicit QtConnection(QtPort *basePort, QGraphicsPathItem *parent = nullptr);
	~QtConnection() = default;

	void setSelected(bool selected); /* TEST */

	/* Redraw the path of the spline */
	void updatePath(const QPointF &altTargetPos = QPointF(0.0f, 0.0f));

	/* Set the base port (needed when the connection is attached to another base port) */
	void setBasePort(QtPort *basePort);
	QtPort *getBasePort() const;

	/* Set the target port */
	void setTargetPort(QtPort *targetPort);
	QtPort *getTargetPort() const;

	/* Set the color of the line */
	void setColor(const QColor &color);

	/* Determines whether a given node is connected to this connection */
	bool isNodeConnectedToThisConnection(QtNode *node);
};
