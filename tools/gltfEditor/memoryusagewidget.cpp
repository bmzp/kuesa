/*
    memoryusage.cpp

    This file is part of Kuesa.

    Copyright (C) 2018-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Juan Casafranca <juan.casafranca@kdab.com.com>

    Licensees holding valid proprietary KDAB Kuesa licenses may use this file in
    accordance with the Kuesa Enterprise License Agreement provided with the Software in the
    LICENSE.KUESA.ENTERPRISE file.

    Contact info@kdab.com if any conditions of this licensing are not clear to you.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "memoryusagewidget.h"
#include "ui_memoryusagewidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QString>

#include <Kuesa/SceneEntity>

#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QGeometry>

#include <Kuesa/private/textureparser_p.h>
#include "textureinspector.h"
#include "utils.h"

namespace {
const QLatin1String LASTPATHSETTING("mainwindow/lastPath");
}

MemoryUsageWidget::MemoryUsageWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MemoryUsageWidget)
    , m_sceneEntity(nullptr)
{
    m_ui->setupUi(this);
}

MemoryUsageWidget::~MemoryUsageWidget()
{
}

void MemoryUsageWidget::setSceneEntity(Kuesa::SceneEntity *sceneEntity)
{
    m_sceneEntity = sceneEntity;
    updateWidgetValues();
}

void MemoryUsageWidget::updateWidgetValues()
{
    updateGeometryMemoryUsage();

    Kuesa::TextureCollection *texturesCollection = m_sceneEntity->textures();
    const auto &names = texturesCollection->names();

    // Textures are loaded by the backend, so we dont have yet the data for the
    // texture Instead of calling directly updateTextureMemoryUsage, we wait
    // until we receive a statusChanged notification
    for (const auto &name : qAsConst(names)) {
        Qt3DRender::QAbstractTexture *texture = texturesCollection->find(name);
        QObject::connect(texture, &Qt3DRender::QAbstractTexture::statusChanged,
                         [this] (Qt3DRender::QAbstractTexture::Status status) {
            if (status == Qt3DRender::QAbstractTexture::Status::Ready)
                updateTextureMemoryUsage();
        });
    }
}

void MemoryUsageWidget::updateGeometryMemoryUsage() const
{
    Kuesa::MeshCollection *meshesCollection = m_sceneEntity->meshes();
    const auto &names = meshesCollection->names();

    int geometrySize = 0;
    QVector<Qt3DRender::QBuffer *> visitedBuffer;
    for (const auto &name: names) {
        Qt3DRender::QGeometryRenderer *mesh = meshesCollection->find(name);
        const auto &attributes =  mesh->geometry()->attributes();

        for (Qt3DRender::QAttribute *attribute : attributes) {
            if (!visitedBuffer.contains(attribute->buffer())) {
                geometrySize += attribute->buffer()->data().size();
                visitedBuffer.push_back(attribute->buffer());
            }
        }
    }

    m_ui->geometryUsage->setText(glTFEditor::Utils::totalSizeString(geometrySize));
}

void MemoryUsageWidget::updateTextureMemoryUsage() const
{
    Kuesa::TextureCollection *texturesCollection = m_sceneEntity->textures();
    const auto &names = texturesCollection->names();

    int texturesSize = 0;
    QVector<Qt3DRender::QAbstractTextureImage *> visitedImages;
    for (const auto &name : qAsConst(names)) {
        Qt3DRender::QAbstractTexture *texture = texturesCollection->find(name);
        Qt3DRender::QAbstractTextureImage *image = texture->textureImages()[0];
        if (!visitedImages.contains(image)) {
            visitedImages.push_back(image);
            texturesSize += ::textureSizeInBytes(texture);
        }
    }

    m_ui->textureUsage->setText(glTFEditor::Utils::totalSizeString(texturesSize));
}
