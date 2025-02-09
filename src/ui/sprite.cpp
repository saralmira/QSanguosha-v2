#include "sprite.h"
#include <QPropertyAnimation>
#include <QPainter>

EffectAnimation::EffectAnimation()
    : QObject()
{
    effects.clear();
    registered.clear();
}

void EffectAnimation::fade(QGraphicsItem *map)
{
    QAnimatedEffect *effect = qobject_cast<QAnimatedEffect *>(map->graphicsEffect());
    if (effect) {
        effectOut(map);
        effect = registered.value(map);
        if (effect) effect->deleteLater();
        registered.insert(map, new FadeEffect(true, this));
        return;
    }

    map->show();
    FadeEffect *fade = new FadeEffect(true, this);
    map->setGraphicsEffect(fade);
    effects.insert(map, fade);
}

void EffectAnimation::emphasize(QGraphicsItem *map, bool stay)
{
    QAnimatedEffect *effect = qobject_cast<QAnimatedEffect *>(map->graphicsEffect());
    if (effect) {
        effectOut(map);
        effect = registered.value(map);
        if (effect) effect->deleteLater();
        registered.insert(map, new EmphasizeEffect(stay, this));
        return;
    }

    EmphasizeEffect *emphasize = new EmphasizeEffect(stay, this);
    map->setGraphicsEffect(emphasize);
    effects.insert(map, emphasize);
}

void EffectAnimation::emphasizefade(QGraphicsItem *map, bool stay)
{
    QAnimatedEffect *effect = qobject_cast<QAnimatedEffect *>(map->graphicsEffect());
    if (effect) {
        effectOut(map);
        effect = registered.value(map);
        if (effect) effect->deleteLater();
        registered.insert(map, new EmphasizeFadeEffect(stay, this));
        return;
    }

    EmphasizeFadeEffect *emphasize = new EmphasizeFadeEffect(stay, this);
    map->setGraphicsEffect(emphasize);
    effects.insert(map, emphasize);
}

void EffectAnimation::sendBack(QGraphicsItem *map)
{
    QAnimatedEffect *effect = qobject_cast<QAnimatedEffect *>(map->graphicsEffect());
    if (effect) {
        effectOut(map);
        effect = registered.value(map);
        if (effect) effect->deleteLater();
        registered.insert(map, new SentbackEffect(true, this));
        return;
    }

    SentbackEffect *sendBack = new SentbackEffect(true, this);
    map->setGraphicsEffect(sendBack);
    effects.insert(map, sendBack);
}

void EffectAnimation::effectOut(QGraphicsItem *map)
{
    QAnimatedEffect *effect = qobject_cast<QAnimatedEffect *>(map->graphicsEffect());
    if (effect) {
        effect->setStay(false);
        connect(effect, SIGNAL(loop_finished()), this, SLOT(deleteEffect()));
    }

    effect = registered.value(map);
    if (effect) effect->deleteLater();
    registered.insert(map, NULL);
}

void EffectAnimation::deleteEffect()
{
    QAnimatedEffect *effect = qobject_cast<QAnimatedEffect *>(sender());
    deleteEffect(effect);
}

void EffectAnimation::deleteEffect(QAnimatedEffect *effect)
{
    if (!effect) return;
    effect->deleteLater();
    QGraphicsItem *pix = effects.key(effect);
    if (pix) {
        QAnimatedEffect *effect = registered.value(pix);
        if (effect) effect->reset();
        pix->setGraphicsEffect(registered.value(pix));
        effects.insert(pix, registered.value(pix));
        registered.insert(pix, NULL);
    }
}

EmphasizeEffect::EmphasizeEffect(bool stay, QObject *parent)
{
    this->setObjectName("emphasizer");
    this->setParent(parent);
    index = 0;
    this->stay = stay;
    QPropertyAnimation *anim = new QPropertyAnimation(this, "index");
    connect(anim, SIGNAL(valueChanged(QVariant)), this, SLOT(update()));
    anim->setEndValue(40);
    anim->setDuration((40 - index) * 5);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void EmphasizeEffect::draw(QPainter *painter)
{
    QSizeF s = this->sourceBoundingRect().size();
    qreal scale = (-qAbs(index - 50) + 50) / 1000.0;
    scale = 0.1 - scale;

    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset);
    const QRectF target = boundingRect().adjusted(s.width() * scale - 1,
        s.height() * scale,
        -s.width() * scale,
        -s.height() * scale);
    const QRectF source(s.width() * 0.1, s.height() * 0.1, s.width(), s.height());

    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawPixmap(target, pixmap, source);
}

QRectF EmphasizeEffect::boundingRectFor(const QRectF &sourceRect) const
{
    qreal scale = 0.1;
    QRectF rect(sourceRect);
    rect.adjust(-sourceRect.width() * scale,
        -sourceRect.height() * scale,
        sourceRect.width() * scale,
        sourceRect.height() * scale);
    return rect;
}

void QAnimatedEffect::setStay(bool stay)
{
    this->stay = stay;
    if (!stay) {
        QPropertyAnimation *anim = new QPropertyAnimation(this, "index");
        anim->setEndValue(0);
        anim->setDuration(index * 5);

        connect(anim, SIGNAL(finished()), this, SLOT(deleteLater()));
        connect(anim, SIGNAL(valueChanged(QVariant)), this, SLOT(update()));
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

SentbackEffect::SentbackEffect(bool stay, QObject *parent)
{
    grayed = NULL;
    this->setObjectName("backsender");
    this->setParent(parent);
    index = 0;
    this->stay = stay;

    QPropertyAnimation *anim = new QPropertyAnimation(this, "index");
    connect(anim, SIGNAL(valueChanged(QVariant)), this, SLOT(update()));
    anim->setEndValue(40);
    anim->setDuration((40 - index) * 5);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

SentbackEffect::~SentbackEffect()
{
    if (grayed) {
        delete grayed;
        grayed = NULL;
    }
}

QRectF SentbackEffect::boundingRectFor(const QRectF &sourceRect) const
{
    qreal scale = 0.05;
    QRectF rect(sourceRect);
    rect.adjust(-sourceRect.width() * scale,
        -sourceRect.height() * scale,
        sourceRect.width() * scale,
        sourceRect.height() * scale);
    return rect;
}

void SentbackEffect::draw(QPainter *painter)
{
    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset);

    if (!grayed) {
        grayed = new QImage(pixmap.size(), QImage::Format_ARGB32);

        QImage image = pixmap.toImage();
        int width = image.width();
        int height = image.height();
        int gray;

        QRgb col;

        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                col = image.pixel(i, j);
                gray = qGray(col) >> 1;
                grayed->setPixel(i, j, qRgba(gray, gray, gray, qAlpha(col)));
            }
        }
    }

    painter->drawPixmap(offset, pixmap);
    painter->setOpacity((40 - qAbs(index - 40)) / 80.0);
    painter->drawImage(offset, *grayed);

    return;
}

FadeEffect::FadeEffect(bool stay, QObject *parent)
{
    this->setObjectName("fader");
    this->setParent(parent);
    index = 0;
    this->stay = stay;

    QPropertyAnimation *anim = new QPropertyAnimation(this, "index");
    connect(anim, SIGNAL(valueChanged(QVariant)), this, SLOT(update()));
    anim->setEndValue(40);
    anim->setDuration((40 - index) * 5);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void FadeEffect::draw(QPainter *painter)
{
    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset);
    painter->setOpacity(index / 40.0);
    painter->drawPixmap(offset, pixmap);
}

EmphasizeFadeEffect::EmphasizeFadeEffect(bool stay, QObject *parent)
{
    brightened = NULL;
    this->setObjectName("emphasize_fader");
    this->setParent(parent);
    index = 0;
    this->stay = stay;

    QPropertyAnimation *anim = new QPropertyAnimation(this, "index");
    connect(anim, SIGNAL(valueChanged(QVariant)), this, SLOT(update()));
    anim->setEndValue(40);
    anim->setDuration((40 - index) * 5);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

EmphasizeFadeEffect::~EmphasizeFadeEffect()
{
    if (brightened) {
        delete brightened;
        brightened = NULL;
    }
}

QImage duplicateImage(const QImage &img)
{
    if (img.format() != QImage::Format_RGB888) {
        return QImage(img).convertToFormat(QImage::Format_RGB888);
    } else {
        return QImage(img);
    }
}

void lightContrastImage(QImage &img, int light, int Contrast)
{
    uchar *rgb = img.bits();
    if (NULL == rgb) {
        return;
    }
    int r;
    int g;
    int b;
    int size = img.width() * img.height();
    for (int i = 0; i < size ; i++) {
        r = light * 0.01 * rgb[i * 3] - 150 + Contrast;
        g = light * 0.01 * rgb[i * 3 + 1] - 150 + Contrast;
        b = light * 0.01 * rgb[i * 3 + 2]  - 150 + Contrast;
        r = qBound(0, r, 255);
        g = qBound(0, g, 255);
        b = qBound(0, b, 255);
        rgb[i * 3] = r;
        rgb[i * 3 + 1] = g;
        rgb[i * 3 + 2] = b;
    }
}

void EmphasizeFadeEffect::draw(QPainter *painter)
{
    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset);

    if (!brightened) {

        QImage image = pixmap.toImage();
        brightened = new QImage(pixmap.size(), QImage::Format_ARGB32);

        int width = image.width();
        int height = image.height();

        //QRgb col;

        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                //col = image.pixel(i, j);
                brightened->setPixel(i, j, qRgba(255, 240, 57, 170));
            }
        }
    }

    painter->drawPixmap(offset, pixmap);
    painter->setOpacity(qAbs(100 - index) / 200.0);
    painter->drawImage(offset, *brightened);
}
