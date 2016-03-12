/****************************************************************************
**
** Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QTest>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>

#include <Qt3DRender/qgeometryrenderer.h>
#include <Qt3DRender/qgeometryfactory.h>
#include <Qt3DRender/qgeometry.h>
#include <Qt3DRender/qattribute.h>
#include <Qt3DRender/qbuffer.h>

#include "testpostmanarbiter.h"

class TestFactory : public Qt3DRender::QGeometryFactory
{
public:
    explicit TestFactory(int size)
        : m_size(size)
    {}

    Qt3DRender::QGeometry *operator ()() Q_DECL_FINAL
    {
        return Q_NULLPTR;
    }

    bool operator ==(const Qt3DRender::QGeometryFactory &other) const Q_DECL_FINAL
    {
        const TestFactory *otherFactory = functor_cast<TestFactory>(&other);
        if (otherFactory != Q_NULLPTR)
            return otherFactory->m_size == m_size;
        return false;
    }

    QT3D_FUNCTOR(TestFactory)

private:
    int m_size;
};

// We need to call QNode::clone which is protected
// So we sublcass QNode instead of QObject
class tst_QGeometryRenderer: public Qt3DCore::QNode
{
    Q_OBJECT
public:
    ~tst_QGeometryRenderer()
    {
        QNode::cleanup();
    }

private Q_SLOTS:

    void checkCloning_data()
    {
        QTest::addColumn<Qt3DRender::QGeometryRenderer *>("geometryRenderer");

        Qt3DRender::QGeometryRenderer *defaultConstructed = new Qt3DRender::QGeometryRenderer();
        QTest::newRow("defaultConstructed") << defaultConstructed ;

        Qt3DRender::QGeometryRenderer *geometry1 = new Qt3DRender::QGeometryRenderer();
        geometry1->setGeometry(new Qt3DRender::QGeometry());
        geometry1->setInstanceCount(1);
        geometry1->setIndexOffset(0);
        geometry1->setFirstInstance(55);
        geometry1->setRestartIndexValue(-1);
        geometry1->setPrimitiveRestartEnabled(false);
        geometry1->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
        geometry1->setVertexCount(15);
        geometry1->setVerticesPerPatch(2);
        geometry1->setGeometryFactory(Qt3DRender::QGeometryFactoryPtr(new TestFactory(383)));
        QTest::newRow("triangle") << geometry1;

        Qt3DRender::QGeometryRenderer *geometry2 = new Qt3DRender::QGeometryRenderer();
        geometry2->setGeometry(new Qt3DRender::QGeometry());
        geometry2->setInstanceCount(200);
        geometry2->setIndexOffset(58);
        geometry2->setFirstInstance(10);
        geometry2->setRestartIndexValue(65535);
        geometry2->setVertexCount(2056);
        geometry2->setPrimitiveRestartEnabled(true);
        geometry2->setVerticesPerPatch(3);
        geometry2->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
        geometry2->setGeometryFactory(Qt3DRender::QGeometryFactoryPtr(new TestFactory(305)));
        QTest::newRow("lines with restart") << geometry2;
    }

    void checkCloning()
    {
        // GIVEN
        QFETCH(Qt3DRender::QGeometryRenderer *, geometryRenderer);

        // WHEN
        Qt3DRender::QGeometryRenderer *clone = static_cast<Qt3DRender::QGeometryRenderer *>(QNode::clone(geometryRenderer));

        // THEN
        QVERIFY(clone != Q_NULLPTR);

        QCOMPARE(clone->id(), geometryRenderer->id());
        QCOMPARE(clone->instanceCount(), geometryRenderer->instanceCount());
        QCOMPARE(clone->vertexCount(), geometryRenderer->vertexCount());
        QCOMPARE(clone->indexOffset(), geometryRenderer->indexOffset());
        QCOMPARE(clone->firstInstance(), geometryRenderer->firstInstance());
        QCOMPARE(clone->restartIndexValue(), geometryRenderer->restartIndexValue());
        QCOMPARE(clone->primitiveRestartEnabled(), geometryRenderer->primitiveRestartEnabled());
        QCOMPARE(clone->primitiveType(), geometryRenderer->primitiveType());
        QCOMPARE(clone->verticesPerPatch(), geometryRenderer->verticesPerPatch());

        if (geometryRenderer->geometry() != Q_NULLPTR) {
            QVERIFY(clone->geometry() != Q_NULLPTR);
            QCOMPARE(clone->geometry()->id(), geometryRenderer->geometry()->id());
        }

        QCOMPARE(clone->geometryFactory(), geometryRenderer->geometryFactory());
        if (geometryRenderer->geometryFactory()) {
            QVERIFY(clone->geometryFactory());
            QVERIFY(*clone->geometryFactory() == *geometryRenderer->geometryFactory());
        }
    }

    void checkPropertyUpdates()
    {
        // GIVEN
        QScopedPointer<Qt3DRender::QGeometryRenderer> geometryRenderer(new Qt3DRender::QGeometryRenderer());
        TestArbiter arbiter(geometryRenderer.data());

        // WHEN
        geometryRenderer->setInstanceCount(256);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        Qt3DCore::QScenePropertyChangePtr change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "instanceCount");
        QCOMPARE(change->value().value<int>(), 256);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        geometryRenderer->setVertexCount(1340);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "vertexCount");
        QCOMPARE(change->value().value<int>(), 1340);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        geometryRenderer->setIndexOffset(883);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "indexOffset");
        QCOMPARE(change->value().value<int>(), 883);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        geometryRenderer->setFirstInstance(1200);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "firstInstance");
        QCOMPARE(change->value().value<int>(), 1200);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        geometryRenderer->setRestartIndexValue(65535);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "restartIndexValue");
        QCOMPARE(change->value().value<int>(), 65535);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        geometryRenderer->setVerticesPerPatch(2);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "verticesPerPatch");
        QCOMPARE(change->value().toInt(), 2);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        geometryRenderer->setPrimitiveRestartEnabled(true);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "primitiveRestartEnabled");
        QCOMPARE(change->value().value<bool>(), true);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        geometryRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Patches);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "primitiveType");
        QCOMPARE(change->value().value<int>(), static_cast<int>(Qt3DRender::QGeometryRenderer::Patches));
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        Qt3DRender::QGeometryFactoryPtr factory(new TestFactory(555));
        geometryRenderer->setGeometryFactory(factory);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "geometryFactory");
        QCOMPARE(change->value().value<Qt3DRender::QGeometryFactoryPtr>(), factory);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        Qt3DRender::QGeometry geom;
        geometryRenderer->setGeometry(&geom);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "geometry");
        QCOMPARE(change->value().value<Qt3DCore::QNodeId>(), geom.id());
        QCOMPARE(change->type(), Qt3DCore::NodeAdded);

        arbiter.events.clear();

        // WHEN
        Qt3DRender::QGeometry geom2;
        geometryRenderer->setGeometry(&geom2);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 2);
        change = arbiter.events.first().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "geometry");
        QCOMPARE(change->value().value<Qt3DCore::QNodeId>(), geom.id());
        QCOMPARE(change->type(), Qt3DCore::NodeRemoved);

        change = arbiter.events.last().staticCast<Qt3DCore::QScenePropertyChange>();
        QCOMPARE(change->propertyName(), "geometry");
        QCOMPARE(change->value().value<Qt3DCore::QNodeId>(), geom2.id());
        QCOMPARE(change->type(), Qt3DCore::NodeAdded);

        arbiter.events.clear();
    }

protected:
    Qt3DCore::QNode *doClone() const Q_DECL_OVERRIDE
    {
        return Q_NULLPTR;
    }

};

QTEST_MAIN(tst_QGeometryRenderer)

#include "tst_qgeometryrenderer.moc"
