#include "gradientpicker.h"
#include "gradient.h"
#include "widgets.h"
#include <QBrush>
#include <QDialogButtonBox>
#include <QGraphicsSceneMouseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QVector>

// GradientControlPoint implementation
GradientControlPoint::GradientControlPoint(int index, qreal x, qreal y,
                                           qreal size, const QColor& color)
    : QGraphicsObject(), index(index), size(size), color(color)
{
    setPos(x, y);
    setAcceptedMouseButtons(Qt::NoButton);
    // setCacheMode(QGraphicsItem::NoCache);
}

QRectF GradientControlPoint::boundingRect() const
{
    return QRectF(0, 0, size, size);
}

void GradientControlPoint::paint(QPainter* painter,
                                 const QStyleOptionGraphicsItem* option,
                                 QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Draw upward-pointing triangle
    qreal centerX = size / 2.0;
    qreal triangleHeight = size * 0.5;
    qreal triangleWidth = size;

    QPolygonF triangle;
    triangle << QPointF(centerX, -3) // Top point
             << QPointF(centerX - triangleWidth / 2,
                        triangleHeight) // Bottom left
             << QPointF(centerX + triangleWidth / 2,
                        triangleHeight); // Bottom right

    painter->setBrush(QBrush(Qt::black));
    painter->setPen(Qt::NoPen);
    painter->drawPolygon(triangle);

    painter->setBrush(QBrush(color));
    painter->setPen(QPen(Qt::black, 2));
    painter->drawEllipse(boundingRect());
}

// GradientSlider implementation
GradientSlider::GradientSlider() : QGraphicsView() { this->initUI(); }

void GradientSlider::setGradient(const Gradient& gradient)
{
    this->gradient = gradient;
    updateGradientDisplay();
    selectFirstPoint();
}

void GradientSlider::setPointColor(const QColor& color)
{
    if (selectedPointIndex >= 0 &&
        selectedPointIndex < gradient.points.size() &&
        selectedPointIndex < controlPoints.size()) {
        gradient.points[selectedPointIndex].color = color;

        // Update control point color
        controlPoints[selectedPointIndex]->color = color;
        controlPoints[selectedPointIndex]->update();

        // Update gradient display
        QLinearGradient linearGrad(10, 0, sliderWidth + 10, 0);
        for (const auto& point : gradient.points) {
            linearGrad.setColorAt(point.position, point.color);
        }
        gradientRect->setBrush(QBrush(linearGrad));

        emit onGradientChanged(gradient);
    }
}

void GradientSlider::selectFirstPoint()
{
    if (gradient.points.size() > 0) {
        selectedPointIndex = 0;
        emit onActivePointChanged(0, gradient.points[0].color);
    }
}

void GradientSlider::initUI()
{
    setMinimumSize(200, 60);
    setFixedHeight(60);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scene = new QGraphicsScene(this);
    setScene(scene);

    // Update slider width based on initial size
    sliderWidth = qMax(200.0, width() - 40.0);
    scene->setSceneRect(0, 0, width() - 20, 40);

    // Create gradient display rectangle
    gradientRect =
        new QGraphicsRectItem(10, sliderY, sliderWidth, sliderHeight);
    gradientRect->setPen(QPen(Qt::black, 1));
    scene->addItem(gradientRect);

    // Initialize with default gradient
    gradient = Gradient::defaultGradient();
    updateGradientDisplay();
    // selectFirstPoint();
}

void GradientSlider::updateGradientDisplay()
{
    // Clear existing control points
    for (auto* point : controlPoints) {
        scene->removeItem(point);
        delete point;
    }
    controlPoints.clear();

    // Create linear gradient for display
    QLinearGradient linearGrad(10, 0, sliderWidth + 10, 0);
    for (const auto& point : gradient.points) {
        linearGrad.setColorAt(point.position, point.color);
    }

    gradientRect->setBrush(QBrush(linearGrad));

    // Create control points
    for (int i = 0; i < gradient.points.size(); ++i) {
        qreal x = xFromPosition(gradient.points[i].position);
        GradientControlPoint* controlPoint = new GradientControlPoint(
            i, x - pointSize / 2, sliderY + sliderHeight, pointSize,
            gradient.points[i].color);
        controlPoints.append(controlPoint);
        scene->addItem(controlPoint);
    }
}

void GradientSlider::addControlPoint(float position, const QColor& color)
{
    gradient.addPoint(GradientPoint(position, color));
    updateGradientDisplay();
    emit onGradientChanged(gradient);
}

void GradientSlider::removeControlPoint(int index)
{
    if (index >= 0 && index < gradient.points.size()) {
        gradient.points.remove(index);
        updateGradientDisplay();
        emit onGradientChanged(gradient);
        selectedPointIndex = -1;
    }
}

void GradientSlider::updateControlPointPositions()
{
    for (int i = 0; i < controlPoints.size() && i < gradient.points.size();
         ++i) {
        qreal x = controlPoints[i]->pos().x() + 5; // Center of control point
        gradient.points[i].position = positionFromX(x);
    }
    updateGradientDisplay();
    emit onGradientChanged(gradient);
}

float GradientSlider::positionFromX(qreal x)
{
    return qBound(0.0, (x - 10.0) / sliderWidth, 1.0);
}

qreal GradientSlider::xFromPosition(float position)
{
    return 10.0 + position * sliderWidth;
}

void GradientSlider::mousePressEvent(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());

    // Check if clicked on a control point
    QGraphicsItem* item = scene->itemAt(scenePos, transform());
    GradientControlPoint* controlPoint =
        dynamic_cast<GradientControlPoint*>(item);

    const int SLIDER_BOTTOM_MARGIN = 15;

    if (event->button() == Qt::LeftButton) {
        if (controlPoint) {
            // Select and start dragging the control point
            selectedPointIndex = controlPoint->index;
            draggingPointIndex = controlPoint->index;
            dragStartPos = scenePos;
            if (selectedPointIndex >= 0 &&
                selectedPointIndex < gradient.points.size()) {
                emit onActivePointChanged(
                    selectedPointIndex,
                    gradient.points[selectedPointIndex].color);
            }
            event->accept();
            return;
        }
        else if (scenePos.y() >= sliderY &&
                 scenePos.y() <=
                     sliderY + sliderHeight + SLIDER_BOTTOM_MARGIN &&
                 scenePos.x() >= 10 && scenePos.x() <= sliderWidth + 10) {
            // Add new control point
            float position = positionFromX(scenePos.x());

            // Sample color at this position
            QColor color = gradient.sample(position);

            addControlPoint(position, color);

            // Select the newly added control point and prepare for dragging
            selectedPointIndex = gradient.points.size() - 1;
            draggingPointIndex = selectedPointIndex;
            dragStartPos = scenePos;

            emit onActivePointChanged(selectedPointIndex, color);

            event->accept();
            return;
        }
    }
    else if (event->button() == Qt::MiddleButton) {
        if (controlPoint) {
            // Delete the control point
            removeControlPoint(controlPoint->index);
            event->accept();
            return;
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void GradientSlider::mouseMoveEvent(QMouseEvent* event)
{
    if (draggingPointIndex >= 0 && draggingPointIndex < controlPoints.size()) {
        QPointF scenePos = mapToScene(event->pos());

        // Constrain to horizontal movement within bounds
        qreal minX = 10;
        qreal maxX = sliderWidth + 10;
        qreal newX = qBound(minX, scenePos.x(), maxX);
        // qDebug() << "Dragging to X:" << newX;

        // Update control point position
        controlPoints[draggingPointIndex]->setPos(newX - pointSize / 2,
                                                  sliderY + sliderHeight);

        controlPoints[draggingPointIndex]->update();
        // qDebug() << "Control point scenePos:"
        //          << controlPoints[draggingPointIndex]->scenePos()
        //          << "pos:" << controlPoints[draggingPointIndex]->pos();

        // Update gradient point position
        gradient.points[draggingPointIndex].position = positionFromX(newX);

        // Update gradient display (without sorting to maintain indices)
        QLinearGradient linearGrad(10, 0, sliderWidth + 10, 0);
        for (const auto& point : gradient.points) {
            linearGrad.setColorAt(point.position, point.color);
        }
        gradientRect->setBrush(QBrush(linearGrad));

        event->accept();
    }
    else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void GradientSlider::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && draggingPointIndex >= 0) {
        draggingPointIndex = -1;
        updateGradientDisplay();
        emit onGradientChanged(gradient);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void GradientSlider::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    // Update slider width based on new size
    sliderWidth = qMax(200.0, width() - 40.0);
    scene->setSceneRect(0, 0, width() - 20, 40);

    // Update gradient rectangle size
    if (gradientRect) {
        gradientRect->setRect(10, sliderY, sliderWidth, sliderHeight);
        updateGradientDisplay();
    }
}

GradientPickerDialog::GradientPickerDialog() { this->initUI(); }

void GradientPickerDialog::setGradient(const Gradient& gradient)
{
    currentGradient = gradient;
    this->gradientSlider->setGradient(gradient);

    // Enable and update color pickers with first point
    if (gradient.points.size() > 0) {
        onControlPointSelected(0, gradient.points[0].color);
    }
}

void GradientPickerDialog::initUI()
{
    setWindowTitle("Gradient Picker");
    setMinimumSize(400, 290);
    resize(460, 290);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    // Create and add gradient slider
    gradientSlider = new GradientSlider();
    layout->addWidget(gradientSlider);

    // Add SVBox and HueSlider
    svBox = new SVBox();
    svBox->setEnabled(false);
    svBox->setMaximumHeight(150); // 60% reduction from default 300px
    svBox->setMinimumHeight(150); // 60% reduction from default 300px
    svBox->setFixedHeight(150);
    layout->addWidget(svBox);

    hueSlider = new HueSlider();
    hueSlider->setEnabled(false);
    hueSlider->setFixedHeight(30);
    layout->addWidget(hueSlider);

    // Add OK and Cancel buttons
    auto buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    setLayout(layout);

    // Connect signals
    connect(gradientSlider, &GradientSlider::onGradientChanged, this,
            [this](const Gradient& gradient) { currentGradient = gradient; });
    connect(gradientSlider, &GradientSlider::onActivePointChanged, this,
            &GradientPickerDialog::onControlPointSelected);
    connect(svBox, &SVBox::onSVChanged, this,
            &GradientPickerDialog::onSVChanged);
    connect(hueSlider, &HueSlider::onHueChanged, this,
            &GradientPickerDialog::onHueChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this,
            &GradientPickerDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this,
            &GradientPickerDialog::onRejected);

    setGradient(Gradient::defaultGradient());
}

void GradientPickerDialog::onControlPointSelected(int index,
                                                  const QColor& color)
{
    Q_UNUSED(index);
    updatingFromControlPoint = true;
    svBox->setEnabled(true);
    svBox->setColor(color);
    hueSlider->setEnabled(true);
    hueSlider->setColor(color);
    updatingFromControlPoint = false;
}

void GradientPickerDialog::onSVChanged(float saturation, float value)
{
    if (updatingFromControlPoint)
        return;

    QColor currentColor = svBox->getColor();
    gradientSlider->setPointColor(currentColor);
}

void GradientPickerDialog::onHueChanged(float hue)
{
    if (updatingFromControlPoint)
        return;

    QColor currentColor = svBox->getColor();
    currentColor.setHsvF(hue, currentColor.saturationF(),
                         currentColor.valueF());
    svBox->setColor(currentColor);
    gradientSlider->setPointColor(currentColor);
}

void GradientPickerDialog::onAccepted()
{
    // Create a sorted clone of the gradient
    Gradient sortedGradient(currentGradient);
    sortedGradient.sort();

    emit onGradientAccepted(sortedGradient);
    accept();
}

void GradientPickerDialog::onRejected() { reject(); }