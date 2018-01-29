/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
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
#include <Qt3DCore/qnode.h>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/qcomponent.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/qpropertyupdatedchange.h>
#include <Qt3DCore/qcomponentaddedchange.h>
#include <Qt3DCore/qcomponentremovedchange.h>
#include <Qt3DCore/qnodedestroyedchange.h>
#include <Qt3DCore/qpropertynodeaddedchange.h>
#include <Qt3DCore/qpropertynoderemovedchange.h>
#include <Qt3DCore/private/qnodecreatedchangegenerator_p.h>
#include <private/qpostman_p.h>

#include <Qt3DCore/private/qlockableobserverinterface_p.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qcomponent_p.h>
#include <QSignalSpy>
#include "testpostmanarbiter.h"

class tst_Nodes : public QObject
{
    Q_OBJECT
public:
    tst_Nodes() : QObject()
    {
        qRegisterMetaType<Qt3DCore::QNode*>();
    }
    ~tst_Nodes() {}

private slots:
    void initTestCase();
    void defaultNodeConstruction();
    void defaultComponentConstruction();
    void defaultEntityConstrution();

    void appendSingleChildNodeToNodeNoSceneExplicitParenting();
    void appendSingleChildNodeToNodeNoSceneImplicitParenting();
    void appendMultipleChildNodesToNodeNoScene();

    void appendSingleChildNodeToNodeSceneExplicitParenting();
    void appendSingleChildNodeToNodeSceneImplicitParenting();
    void appendMultipleChildNodesToNodeScene();

    void checkParentChangeToNull();
    void checkParentChangeToOtherParent();

    void removingSingleChildNodeFromNode();
    void removingMultipleChildNodesFromNode();

    void appendingChildEntitiesToNode();
    void removingChildEntitiesFromNode();

    void checkConstructionSetParentMix(); // QTBUG-60612
    void checkConstructionWithParent();
    void checkConstructionAsListElement();

    void appendingComponentToEntity();
    void appendingParentlessComponentToEntity();
    void removingComponentFromEntity();

    void changeCustomProperty();
    void checkDestruction();

    void checkDefaultConstruction();
    void checkPropertyChanges();
    void checkCreationData();
    void checkEnabledUpdate();
    void checkPropertyTrackModeUpdate();
    void checkTrackedPropertyNamesUpdate();
};

class ObserverSpy;
class SimplePostman : public Qt3DCore::QAbstractPostman
{
public:
    SimplePostman(ObserverSpy *spy)
        : m_spy(spy)
    {}

    void sceneChangeEvent(const Qt3DCore::QSceneChangePtr &) Q_DECL_FINAL {};
    void setScene(Qt3DCore::QScene *) Q_DECL_FINAL {};
    void notifyBackend(const Qt3DCore::QSceneChangePtr &change) Q_DECL_FINAL;
    bool shouldNotifyFrontend(const Qt3DCore::QSceneChangePtr &changee) Q_DECL_FINAL { Q_UNUSED(changee); return false; }

private:
    ObserverSpy *m_spy;
};

class ObserverSpy : public Qt3DCore::QAbstractArbiter
{
public:
    class ChangeRecord : public QPair<Qt3DCore::QSceneChangePtr, bool>
    {
    public:
        ChangeRecord(const Qt3DCore::QSceneChangePtr &event, bool locked)
            : QPair<Qt3DCore::QSceneChangePtr, bool>(event, locked)
        {}

        Qt3DCore::QSceneChangePtr change() const { return first; }

        bool wasLocked() const { return second; }
    };

    ObserverSpy()
        : Qt3DCore::QAbstractArbiter()
        , m_postman(new SimplePostman(this))
    {
    }

    ~ObserverSpy()
    {
    }

    void sceneChangeEventWithLock(const Qt3DCore::QSceneChangePtr &e) Q_DECL_OVERRIDE
    {
        events << ChangeRecord(e, true);
    }

    void sceneChangeEventWithLock(const Qt3DCore::QSceneChangeList &e) Q_DECL_OVERRIDE
    {
        for (size_t i = 0, m = e.size(); i < m; ++i) {
            events << ChangeRecord(e.at(i), false);
        }
    }

    void sceneChangeEvent(const Qt3DCore::QSceneChangePtr &e) Q_DECL_OVERRIDE
    {
        events << ChangeRecord(e, false);
    }

    Qt3DCore::QAbstractPostman *postman() const Q_DECL_FINAL
    {
        return m_postman.data();
    }

    QList<ChangeRecord> events;
    QScopedPointer<SimplePostman> m_postman;
};

void SimplePostman::notifyBackend(const Qt3DCore::QSceneChangePtr &change)
{
    m_spy->sceneChangeEventWithLock(change);
}



class MyQNode : public Qt3DCore::QNode
{
    Q_OBJECT
    Q_PROPERTY(QString customProperty READ customProperty WRITE setCustomProperty NOTIFY customPropertyChanged)
    Q_PROPERTY(MyQNode *nodeProperty READ nodeProperty WRITE setNodeProperty NOTIFY nodePropertyChanged)
public:
    explicit MyQNode(Qt3DCore::QNode *parent = 0)
        : QNode(parent)
        , m_nodeProperty(nullptr)
    {}

    ~MyQNode()
    {
    }

    void setCustomProperty(const QString &s)
    {
        if (m_customProperty == s)
            return;

        m_customProperty = s;
        emit customPropertyChanged();
    }

    QString customProperty() const
    {
        return m_customProperty;
    }

    void setArbiterAndScene(Qt3DCore::QAbstractArbiter *arbiter,
                            Qt3DCore::QScene *scene = nullptr)
    {
        Q_ASSERT(arbiter);
        if (scene)
            scene->setArbiter(arbiter);
        Qt3DCore::QNodePrivate::get(this)->setScene(scene);
        Qt3DCore::QNodePrivate::get(this)->setArbiter(arbiter);
    }

    void setSimulateBackendCreated(bool created)
    {
        Qt3DCore::QNodePrivate::get(this)->m_hasBackendNode = created;
    }

    MyQNode *nodeProperty() const { return m_nodeProperty; }

public slots:
    void setNodeProperty(MyQNode *node)
    {
        Qt3DCore::QNodePrivate *d = Qt3DCore::QNodePrivate::get(this);
        if (m_nodeProperty == node)
            return;

        if (m_nodeProperty)
            d->unregisterDestructionHelper(m_nodeProperty);

        if (node && !node->parent())
            node->setParent(this);

        m_nodeProperty = node;

        // Ensures proper bookkeeping
        if (m_nodeProperty)
            d->registerDestructionHelper(m_nodeProperty, &MyQNode::setNodeProperty, m_nodeProperty);

        emit nodePropertyChanged(node);
    }

    void addAttribute(MyQNode *attribute)
    {
        Qt3DCore::QNodePrivate *d = Qt3DCore::QNodePrivate::get(this);
        if (!m_attributes.contains(attribute)) {
            m_attributes.append(attribute);

            // Ensures proper bookkeeping
            d->registerDestructionHelper(attribute, &MyQNode::removeAttribute, m_attributes);

            // We need to add it as a child of the current node if it has been declared inline
            // Or not previously added as a child of the current node so that
            // 1) The backend gets notified about it's creation
            // 2) When the current node is destroyed, it gets destroyed as well
            if (!attribute->parent())
                attribute->setParent(this);

            if (d->m_changeArbiter != nullptr) {
                const auto change = Qt3DCore::QPropertyNodeAddedChangePtr::create(id(), attribute);
                change->setPropertyName("attribute");
                d->notifyObservers(change);
            }
        }
    }

    void removeAttribute(MyQNode *attribute)
    {
        Qt3DCore::QNodePrivate *d = Qt3DCore::QNodePrivate::get(this);
        if (d->m_changeArbiter != nullptr) {
            const auto change = Qt3DCore::QPropertyNodeRemovedChangePtr::create(id(), attribute);
            change->setPropertyName("attribute");
            d->notifyObservers(change);
        }
        m_attributes.removeOne(attribute);
        // Remove bookkeeping connection
        d->unregisterDestructionHelper(attribute);
    }

signals:
    void customPropertyChanged();
    void nodePropertyChanged(MyQNode *node);

protected:
    QString m_customProperty;
    MyQNode *m_nodeProperty;
    QVector<MyQNode *> m_attributes;
};

class MyQEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
public:
    explicit MyQEntity(Qt3DCore::QNode *parent = 0)
        : QEntity(parent)
    {}

    ~MyQEntity()
    {
    }

    void setArbiterAndScene(Qt3DCore::QAbstractArbiter *arbiter,
                            Qt3DCore::QScene *scene = nullptr)
    {
        Q_ASSERT(arbiter);
        if (scene)
            scene->setArbiter(arbiter);
        Qt3DCore::QNodePrivate::get(this)->setScene(scene);
        Qt3DCore::QNodePrivate::get(this)->setArbiter(arbiter);
    }

    void setSimulateBackendCreated(bool created)
    {
        Qt3DCore::QNodePrivate::get(this)->m_hasBackendNode = created;
    }
};

class MyQComponent : public Qt3DCore::QComponent
{
    Q_OBJECT
public:
    explicit MyQComponent(Qt3DCore::QNode *parent = 0) : QComponent(parent)
    {}
    void setArbiter(Qt3DCore::QAbstractArbiter *arbiter)
    {
        Q_ASSERT(arbiter);
        Qt3DCore::QComponentPrivate::get(this)->setArbiter(arbiter);
    }
};


void tst_Nodes::initTestCase()
{
    qRegisterMetaType<Qt3DCore::QNode::PropertyTrackingMode>("PropertyTrackingMode");
}

void tst_Nodes::defaultNodeConstruction()
{
    // GIVEN
    QScopedPointer<MyQNode> node(new MyQNode());

    // THEN
    QVERIFY(node != nullptr);
    QVERIFY(node->children().isEmpty());

    // GIVEN
    MyQNode *node2 = new MyQNode(node.data());
    QCoreApplication::processEvents();

    // THEN
    QVERIFY(node2->parent() == node.data());
    QVERIFY(!node->children().isEmpty());
    QVERIFY(node2->children().isEmpty());
}

void tst_Nodes::defaultComponentConstruction()
{
    // GIVEN
    QScopedPointer<MyQComponent> comp(new MyQComponent());
    MyQComponent *comp2 = new MyQComponent(comp.data());
    QCoreApplication::processEvents();

    // THEN
    QVERIFY(comp != nullptr);
    QCOMPARE(comp2->parent(), comp.data());
}

void tst_Nodes::defaultEntityConstrution()
{
    // GIVEN
    QScopedPointer<Qt3DCore::QEntity> entity(new Qt3DCore::QEntity());
    Qt3DCore::QEntity *entity2 = new Qt3DCore::QEntity(entity.data());
    QCoreApplication::processEvents();

    // THEN
    QVERIFY(entity->components().isEmpty());
    QVERIFY(entity2->components().isEmpty());
    QCOMPARE(entity2->parent(), entity.data());
}

void tst_Nodes::appendSingleChildNodeToNodeNoSceneExplicitParenting()
{
    // Check nodes added when no scene is set
    // GIVEN
    ObserverSpy spy;
    QScopedPointer<MyQNode> node(new MyQNode());
    node->setArbiterAndScene(&spy);

    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(node.data())->scene() == nullptr);
    // WHEN
    QScopedPointer<MyQNode> child(new MyQNode());

    // THEN
    QVERIFY(child->parent() == nullptr);

    // WHEN
    child->setParent(node.data());

    // THEN
    QVERIFY(child->parent() == node.data());
    QVERIFY(child->parentNode() == node.data());
    QCOMPARE(node->children().count(), 1);

    // Events are only sent when a scene is set on the root node
    QCOMPARE(spy.events.size(), 0);
}

void tst_Nodes::appendSingleChildNodeToNodeNoSceneImplicitParenting()
{
    // Check nodes added when no scene is set
    // GIVEN
    ObserverSpy spy;
    QScopedPointer<MyQNode> node(new MyQNode());
    node->setArbiterAndScene(&spy);

    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(node.data())->scene() == nullptr);
    // WHEN
    QScopedPointer<MyQNode> child(new MyQNode(node.data()));
    QCoreApplication::processEvents();

    // THEN
    QVERIFY(child->parent() == node.data());
    QVERIFY(child->parentNode() == node.data());
    QCOMPARE(node->children().count(), 1);

    // Events are only sent when a scene is set on the root node
    QCOMPARE(spy.events.size(), 0);
}

void tst_Nodes::appendMultipleChildNodesToNodeNoScene()
{
    // Check multiple nodes added with no scene set
    // GIVEN
    ObserverSpy spy;
    QScopedPointer<MyQNode> node(new MyQNode());
    node->setArbiterAndScene(&spy);

    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(node.data())->scene() == nullptr);
    // WHEN
    for (int i = 0; i < 10; i++) {
        // WHEN
        Qt3DCore::QNode *child = nullptr;
        if (i % 2 == 0) {
            child = new MyQNode(node.data());
            QCoreApplication::processEvents();
        } else {
            child = new MyQNode();
            child->setParent(node.data());
        }
        // THEN
        QVERIFY(child->parent() == node.data());
    }

    // THEN
    QCOMPARE(node->children().count(), 10);

    // Events are only sent when a scene is set on the root node
    QCOMPARE(spy.events.size(), 0);
}

void tst_Nodes::appendSingleChildNodeToNodeSceneExplicitParenting()
{
    // Check nodes added when scene is set
    // GIVEN
    Qt3DCore::QScene scene;
    ObserverSpy spy;
    QScopedPointer<MyQNode> node(new MyQNode());
    // WHEN
    node->setArbiterAndScene(&spy, &scene);
    node->setSimulateBackendCreated(true);

    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(node.data())->scene() != nullptr);

    // WHEN
    QScopedPointer<MyQNode> child(new MyQNode());

    // THEN
    QVERIFY(child->parent() == nullptr);
    QVERIFY(Qt3DCore::QNodePrivate::get(child.data())->scene() == nullptr);

    // WHEN
    child->setParent(node.data());

    // THEN
    QVERIFY(child->parent() == node.data());
    QVERIFY(child->parentNode() == node.data());
    QCOMPARE(spy.events.size(), 2); // Created + Child Added
    QCOMPARE(node->children().count(), 1);
    QVERIFY(Qt3DCore::QNodePrivate::get(child.data())->scene() != nullptr);

    // Creation event
    QVERIFY(spy.events.first().wasLocked());
    Qt3DCore::QNodeCreatedChangeBasePtr creationEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QNodeCreatedChangeBase>();
    QVERIFY(creationEvent);
    QCOMPARE(creationEvent->subjectId(), child->id());
    QCOMPARE(creationEvent->metaObject(), child->metaObject());
    QCOMPARE(creationEvent->isNodeEnabled(), child->isEnabled());
    QCOMPARE(creationEvent->parentId(), child->parentNode()->id());

    // Node Added event
    QVERIFY(spy.events.first().wasLocked());
    Qt3DCore::QPropertyNodeAddedChangePtr additionEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyNodeAddedChange>();
    QVERIFY(additionEvent);
    QCOMPARE(additionEvent->subjectId(), node->id());
    QCOMPARE(additionEvent->addedNodeId(), child->id());
    QCOMPARE(additionEvent->metaObject(), child->metaObject());
}

void tst_Nodes::appendSingleChildNodeToNodeSceneImplicitParenting()
{
    // Check nodes added when scene is set
    // GIVEN
    Qt3DCore::QScene scene;
    ObserverSpy spy;
    QScopedPointer<MyQNode> node(new MyQNode());

    // WHEN
    node->setArbiterAndScene(&spy, &scene);
    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(node.data())->scene() != nullptr);

    // WHEN
    QScopedPointer<MyQNode> child(new MyQNode(node.data()));
    QCoreApplication::processEvents();

    // THEN
    QVERIFY(child->parent() == node.data());
    QVERIFY(child->parentNode() == node.data());
    QVERIFY(Qt3DCore::QNodePrivate::get(child.data())->scene() != nullptr);

    QCOMPARE(spy.events.size(), 2);
    QVERIFY(spy.events.first().wasLocked());
    QCOMPARE(node->children().count(), 1);

    // Creation event
    QVERIFY(spy.events.first().wasLocked());
    Qt3DCore::QNodeCreatedChangeBasePtr creationEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QNodeCreatedChangeBase>();
    QVERIFY(creationEvent);
    QCOMPARE(creationEvent->subjectId(), child->id());
    QCOMPARE(creationEvent->metaObject(), child->metaObject());
    QCOMPARE(creationEvent->isNodeEnabled(), child->isEnabled());
    QCOMPARE(creationEvent->parentId(), child->parentNode()->id());

    // Node Added event
    QVERIFY(spy.events.first().wasLocked());
    Qt3DCore::QPropertyNodeAddedChangePtr additionEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyNodeAddedChange>();
    QVERIFY(additionEvent);
    QCOMPARE(additionEvent->subjectId(), node->id());
    QCOMPARE(additionEvent->addedNodeId(), child->id());
    QCOMPARE(additionEvent->metaObject(), child->metaObject());
}

void tst_Nodes::appendMultipleChildNodesToNodeScene()
{
    // Check nodes added when scene is set

    // GIVEN
    Qt3DCore::QScene scene;
    ObserverSpy spy;
    QScopedPointer<MyQNode> node(new MyQNode());

    // WHEN
    node->setArbiterAndScene(&spy, &scene);
    node->setSimulateBackendCreated(true);
    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(node.data())->scene() != nullptr);

    // WHEN
    const auto childCount = 10;
    for (int i = 0; i < childCount; i++) {
        // WHEN
        Qt3DCore::QNode *child = nullptr;
        if (i % 2 == 0) {
            child = new MyQNode(node.data());
        } else {
            child = new MyQNode();
            child->setParent(node.data());
        }

        // THEN parent and scene should be set synchronously
        QVERIFY(child->parent() == node.data());
        QVERIFY(Qt3DCore::QNodePrivate::get(child)->scene() == Qt3DCore::QNodePrivate::get(node.data())->m_scene);
    }
    // THEN
    QCOMPARE(node->children().count(), 10);

    // WHEN
    QCoreApplication::processEvents();

    // THEN backend is notified after the event loop spins. The recorded events are a little
    // tricky to understand and differs for children with the parent being set at construction
    // time (even children and ids) and the children being created without a parent and then
    // explicitly calling setParent() after (odd children and ids).
    //
    // Even children:
    //   child constructed
    //   notifications to backend scheduled via the event loop as object is not yet fully constructed
    //
    // Odd children:
    //   child constructed
    //   parent set
    //   notifications to backend sent immediately as object is fully constructed
    //
    // With this in mind, the recorded events should show:
    //
    // for each odd child:
    //   odd child creation
    //   child addition to parent of odd child
    //
    // followed by:
    //
    // for each even child:
    //   even child construction
    //   child addition to parent of even child
    //
    const auto expectedEventCount = 2 * childCount;
    QCOMPARE(spy.events.size(), 10 * 2);

    for (auto i = 0; i < expectedEventCount; i += 2) {
        const auto creationRecord = spy.events.at(i);
        QVERIFY(creationRecord.wasLocked());
        const auto childIndex = i < 10
                ? (i / 2) * 2 + 1
                : (i - 10) / 2 * 2;
        Qt3DCore::QNode *child = node->childNodes().at(childIndex);

        const Qt3DCore::QNodeCreatedChangeBasePtr event = creationRecord.change().dynamicCast<Qt3DCore::QNodeCreatedChangeBase>();
        QVERIFY(event != nullptr);
        QCOMPARE(event->subjectId(), child->id());
        QCOMPARE(event->metaObject(), child->metaObject());
        QCOMPARE(event->isNodeEnabled(), child->isEnabled());

        const auto additionRecord = spy.events.at(i + 1);
        Qt3DCore::QPropertyNodeAddedChangePtr additionEvent = additionRecord.change().dynamicCast<Qt3DCore::QPropertyNodeAddedChange>();
        QCOMPARE(additionEvent->subjectId(), node->id());
        QCOMPARE(additionEvent->addedNodeId(), child->id());
        QCOMPARE(additionEvent->metaObject(), child->metaObject());
    }
}

void tst_Nodes::checkParentChangeToNull()
{
    // GIVEN
    Qt3DCore::QScene scene;
    ObserverSpy spy;
    QScopedPointer<MyQNode> root(new MyQNode());

    // WHEN
    root->setArbiterAndScene(&spy, &scene);
    QScopedPointer<Qt3DCore::QNode> child(new MyQNode(root.data()));
    QCoreApplication::processEvents();

    // THEN
    QVERIFY(child->parent() == root.data());
    QCOMPARE(spy.events.size(), 2);
    QCOMPARE(root->children().size(), 1);

    // WHEN
    spy.events.clear();
    child->setParent(Q_NODE_NULLPTR);

    // THEN
    QVERIFY(child->parent() == nullptr);
    QCOMPARE(root->children().size(), 0);
    QCOMPARE(spy.events.size(), 2);

    QVERIFY(spy.events.first().wasLocked());
    const Qt3DCore::QPropertyNodeRemovedChangePtr removalEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyNodeRemovedChange>();
    QVERIFY(removalEvent);
    QCOMPARE(removalEvent->subjectId(), root->id());
    QCOMPARE(removalEvent->removedNodeId(), child->id());
    QCOMPARE(removalEvent->metaObject(), child->metaObject());

    QVERIFY(spy.events.first().wasLocked());
    const Qt3DCore::QNodeDestroyedChangePtr destructionEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QNodeDestroyedChange>();
    QVERIFY(destructionEvent);
    QCOMPARE(destructionEvent->type(), Qt3DCore::NodeDeleted);
    QCOMPARE(destructionEvent->subjectId(), child->id());
    QCOMPARE(destructionEvent->subtreeIdsAndTypes().size(), 1);
    QCOMPARE(destructionEvent->subtreeIdsAndTypes().first().id, child->id());
    QCOMPARE(destructionEvent->subtreeIdsAndTypes().first().type, child->metaObject());
}

void tst_Nodes::checkParentChangeToOtherParent()
{
    // GIVEN
    Qt3DCore::QScene scene;
    ObserverSpy spy;
    QScopedPointer<MyQNode> root(new MyQNode());
    root->setArbiterAndScene(&spy, &scene);
    QScopedPointer<MyQNode> parent1(new MyQNode(root.data()));
    QScopedPointer<MyQNode> parent2(new MyQNode(root.data()));
    QCoreApplication::processEvents();

    // THEN
    QCOMPARE(spy.events.size(), 4); // 2 x (1 node created change, 1 node added to children change)

    // WHEN
    spy.events.clear();
    QScopedPointer<Qt3DCore::QNode> child(new MyQNode(parent1.data()));
    QCoreApplication::processEvents();

    // THEN
    QVERIFY(child->parent() == parent1.data());
    QCOMPARE(parent1->children().size(), 1);
    QCOMPARE(parent2->children().size(), 0);
    QVERIFY(Qt3DCore::QNodePrivate::get(child.data())->scene() != nullptr);
    QCOMPARE(spy.events.size(), 2); // 1 node created change, 1 node added to children change

    // WHEN
    spy.events.clear();
    child->setParent(parent2.data());

    // THEN
    QVERIFY(child->parent() == parent2.data());
    QCOMPARE(parent1->children().size(), 0);
    QCOMPARE(parent2->children().size(), 1);
    QCOMPARE(spy.events.size(), 2);

    // CHECK event 1 is a Node Removed event
    QVERIFY(spy.events.first().wasLocked());
    const Qt3DCore::QPropertyNodeRemovedChangePtr event = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyNodeRemovedChange>();
    QCOMPARE(event->type(), Qt3DCore::PropertyValueRemoved);
    QCOMPARE(event->subjectId(), parent1->id());
    QCOMPARE(event->removedNodeId(), child->id());
    QCOMPARE(event->metaObject(), child->metaObject());

    // CHECK event 2 is a Node Added event
    QVERIFY(spy.events.last().wasLocked());
    const Qt3DCore::QPropertyNodeAddedChangePtr event2 = spy.events.last().change().dynamicCast<Qt3DCore::QPropertyNodeAddedChange>();
    QCOMPARE(event2->type(), Qt3DCore::PropertyValueAdded);
    QCOMPARE(event2->subjectId(), parent2->id());
    QCOMPARE(event2->addedNodeId(), child->id());
    QCOMPARE(event2->metaObject(), child->metaObject());
}

void tst_Nodes::removingSingleChildNodeFromNode()
{
    // GIVEN
    Qt3DCore::QScene scene;
    ObserverSpy spy;
    QScopedPointer<MyQNode> root(new MyQNode());
    QScopedPointer<Qt3DCore::QNode> child(new MyQNode());

    // WHEN
    root->setArbiterAndScene(&spy, &scene);
    root->setSimulateBackendCreated(true);
    child->setParent(root.data());

    // Clear any creation event
    spy.events.clear();

    // THEN
    QVERIFY(root->children().count() == 1);
    QVERIFY(child->parentNode() == root.data());

    // WHEN
    child->setParent(Q_NODE_NULLPTR);

    // THEN
    QVERIFY(child->parent() == nullptr);
    QVERIFY(root->children().count() == 0);

    QCOMPARE(spy.events.size(), 2);

    QVERIFY(spy.events.first().wasLocked());
    const Qt3DCore::QPropertyNodeRemovedChangePtr removalEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyNodeRemovedChange>();
    QCOMPARE(removalEvent->subjectId(), root->id());
    QCOMPARE(removalEvent->removedNodeId(), child->id());
    QCOMPARE(removalEvent->metaObject(), child->metaObject());

    QVERIFY(spy.events.first().wasLocked());
    const Qt3DCore::QNodeDestroyedChangePtr destructionEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QNodeDestroyedChange>();
    QCOMPARE(destructionEvent->type(), Qt3DCore::NodeDeleted);
    QCOMPARE(destructionEvent->subjectId(), child->id());
    QCOMPARE(destructionEvent->subtreeIdsAndTypes().size(), 1);
    QCOMPARE(destructionEvent->subtreeIdsAndTypes().first().id, child->id());
    QCOMPARE(destructionEvent->subtreeIdsAndTypes().first().type, child->metaObject());
}

void tst_Nodes::removingMultipleChildNodesFromNode()
{
    // GIVEN
    Qt3DCore::QScene scene;
    ObserverSpy spy;
    QScopedPointer<MyQNode> root(new MyQNode());

    // WHEN
    root->setArbiterAndScene(&spy, &scene);

    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(root.data())->scene() != nullptr);

    // WHEN
    Qt3DCore::QNodeIdVector childIds(10);
    for (int i = 0; i < 10; i++) {
        auto child = new MyQNode(root.data());
        childIds[i] = child->id();
    }

    QCoreApplication::processEvents();

    // THEN
    QCOMPARE(root->children().count(), 10);
    QCOMPARE(spy.events.size(), 20);

    // WHEN
    spy.events.clear();
    Q_FOREACH (QObject *c, root->children())
        delete c;

    // THEN
    QVERIFY(root->children().count() == 0);
    QCOMPARE(spy.events.size(), 20);
    int i = 0;
    Q_FOREACH (const ObserverSpy::ChangeRecord &r, spy.events) {
        QVERIFY(r.wasLocked());
        const Qt3DCore::QNodeId childId = childIds.at(i / 2);
        if (i % 2 == 0) {
            Qt3DCore::QPropertyNodeRemovedChangePtr additionEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyNodeRemovedChange>();
            QCOMPARE(additionEvent->subjectId(), root->id());
            QCOMPARE(additionEvent->removedNodeId(), childId);
            QCOMPARE(additionEvent->metaObject(), &MyQNode::staticMetaObject);
        } else {
            const Qt3DCore::QNodeDestroyedChangePtr event = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QNodeDestroyedChange>();
            QCOMPARE(event->subjectId(), childId);
            QCOMPARE(event->subtreeIdsAndTypes().count(), 1);
            QCOMPARE(event->subtreeIdsAndTypes().first().id, childId);
            QCOMPARE(event->subtreeIdsAndTypes().first().type, &MyQNode::staticMetaObject);
            QCOMPARE(event->type(), Qt3DCore::NodeDeleted);
        }
        ++i;
    }
}

void tst_Nodes::appendingChildEntitiesToNode()
{
    // GIVEN
    QScopedPointer<MyQNode> root(new MyQNode());

    // WHEN
    Qt3DCore::QEntity *childEntity = new Qt3DCore::QEntity(root.data());
    QCoreApplication::processEvents();

    // THEN
    QVERIFY(root->children().first() == childEntity);
    QVERIFY(childEntity->parentEntity() == nullptr);
    QVERIFY(childEntity->parentNode() == root.data());
}

void tst_Nodes::removingChildEntitiesFromNode()
{
    // GIVEN
    QScopedPointer<MyQNode> root(new MyQNode());

    // WHEN
    Qt3DCore::QEntity *childEntity = new Qt3DCore::QEntity(root.data());
    QCoreApplication::processEvents();

    // THEN
    QVERIFY(root->children().first() == childEntity);
    QVERIFY(childEntity->parentEntity() == nullptr);
    QVERIFY(childEntity->parentNode() == root.data());

    // WHEN
    childEntity->setParent(Q_NODE_NULLPTR);

    // THEN
    QVERIFY(root->children().isEmpty());
    QVERIFY(childEntity->parentNode() == nullptr);
    QVERIFY(childEntity->parent() == nullptr);
}

void tst_Nodes::checkConstructionSetParentMix()
{
    // GIVEN
    ObserverSpy spy;
    Qt3DCore::QScene scene;
    QScopedPointer<MyQNode> root(new MyQNode());

    // WHEN
    root->setArbiterAndScene(&spy, &scene);
    root->setSimulateBackendCreated(true);

    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(root.data())->scene() != nullptr);

    // WHEN
    Qt3DCore::QEntity *subTreeRoot = new Qt3DCore::QEntity(root.data());

    for (int i = 0; i < 100; ++i) {
        Qt3DCore::QEntity *child = new Qt3DCore::QEntity();
        child->setParent(subTreeRoot);
    }

    // THEN
    QCoreApplication::processEvents();
    QCOMPARE(root->children().count(), 1);
    QCOMPARE(subTreeRoot->children().count(), 100);
    QCOMPARE(spy.events.size(), 102); // 1 subTreeRoot creation change, 100 child creation, 1 child added (subTree to root)

    // Ensure first event is subTreeRoot
    const Qt3DCore::QNodeCreatedChangeBasePtr firstEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QNodeCreatedChangeBase>();
    QVERIFY(!firstEvent.isNull());
    QCOMPARE(firstEvent->subjectId(), subTreeRoot->id());
    QCOMPARE(firstEvent->parentId(), root->id());

    const Qt3DCore::QPropertyNodeAddedChangePtr lastEvent = spy.events.takeLast().change().dynamicCast<Qt3DCore::QPropertyNodeAddedChange>();
    QVERIFY(!lastEvent.isNull());
    QCOMPARE(lastEvent->subjectId(), root->id());
    QCOMPARE(lastEvent->propertyName(), "children");
    QCOMPARE(lastEvent->addedNodeId(), subTreeRoot->id());
}

void tst_Nodes::checkConstructionWithParent()
{
    // GIVEN
    ObserverSpy spy;
    Qt3DCore::QScene scene;
    QScopedPointer<MyQNode> root(new MyQNode());

    // WHEN
    root->setArbiterAndScene(&spy, &scene);
    root->setSimulateBackendCreated(true);

    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(root.data())->scene() != nullptr);

    // WHEN we create a child and then set it as a Node* property
    auto *node = new MyQNode(root.data());
    root->setNodeProperty(node);

    // THEN we should get one creation change, one child added change
    // and one property change event, in that order.
    QCoreApplication::processEvents();
    QCOMPARE(root->children().count(), 1);
    QCOMPARE(spy.events.size(), 3); // 1 creation change, 1 child added change, 1 property change

    // Ensure first event is child node's creation change
    const auto creationEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QNodeCreatedChangeBase>();
    QVERIFY(!creationEvent.isNull());
    QCOMPARE(creationEvent->subjectId(), node->id());

    const auto newChildEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyNodeAddedChange>();
    QVERIFY(!newChildEvent.isNull());
    QCOMPARE(newChildEvent->subjectId(), root->id());
    QCOMPARE(newChildEvent->propertyName(), "children");
    QCOMPARE(newChildEvent->addedNodeId(), node->id());

    // Ensure second and last event is property set change
    const auto propertyEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyUpdatedChange>();
    QVERIFY(!propertyEvent.isNull());
    QCOMPARE(propertyEvent->subjectId(), root->id());
    QCOMPARE(propertyEvent->propertyName(), "nodeProperty");
    QCOMPARE(propertyEvent->value().value<Qt3DCore::QNodeId>(), node->id());
}

void tst_Nodes::checkConstructionAsListElement()
{
    // GIVEN
    ObserverSpy spy;
    Qt3DCore::QScene scene;
    QScopedPointer<MyQNode> root(new MyQNode());

    // WHEN
    root->setArbiterAndScene(&spy, &scene);
    root->setSimulateBackendCreated(true);

    // THEN
    QVERIFY(Qt3DCore::QNodePrivate::get(root.data())->scene() != nullptr);

    // WHEN we create a child and then set it as a Node* property
    auto *node = new MyQNode(root.data());
    root->addAttribute(node);

    // THEN we should get one creation change, one child added change
    // and one property change event, in that order.
    QCoreApplication::processEvents();

    QCOMPARE(root->children().count(), 1);
    QCOMPARE(spy.events.size(), 3); // 1 creation change, 1 child added change, 1 property change

    // Ensure first event is child node's creation change
    const auto creationEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QNodeCreatedChangeBase>();
    QVERIFY(!creationEvent.isNull());
    QCOMPARE(creationEvent->subjectId(), node->id());

    const auto newChildEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyNodeAddedChange>();
    QVERIFY(!newChildEvent.isNull());
    QCOMPARE(newChildEvent->subjectId(), root->id());
    QCOMPARE(newChildEvent->propertyName(), "children");
    QCOMPARE(newChildEvent->addedNodeId(), node->id());

    // Ensure second and last event is property set change
    const auto propertyEvent = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyNodeAddedChange>();
    QVERIFY(!propertyEvent.isNull());
    QCOMPARE(propertyEvent->subjectId(), root->id());
    QCOMPARE(propertyEvent->propertyName(), "attribute");
    QCOMPARE(newChildEvent->addedNodeId(), node->id());
}

void tst_Nodes::appendingParentlessComponentToEntity()
{
    // GIVEN
    ObserverSpy entitySpy;
    ObserverSpy componentSpy;
    {
        QScopedPointer<MyQEntity> entity(new MyQEntity());
        entity->setArbiterAndScene(&entitySpy);
        entity->setSimulateBackendCreated(true);

        MyQComponent *comp = new MyQComponent();
        comp->setArbiter(&componentSpy);

        // THEN
        QVERIFY(entity->parentNode() == nullptr);
        QVERIFY(entity->children().count() == 0);
        QVERIFY(entity->components().empty());
        QVERIFY(comp->parentNode() == nullptr);

        // WHEN
        entity->addComponent(comp);

        // THEN
        QVERIFY(entity->components().count() == 1);
        QVERIFY(entity->components().first() == comp);
        QVERIFY(comp->parentNode() == entity.data());
        QCOMPARE(entitySpy.events.size(), 1);
        QVERIFY(entitySpy.events.first().wasLocked());
        QCOMPARE(componentSpy.events.size(), 1);

        // Note: since QEntity has no scene in this test case, we only have the
        // ComponentAdded event In theory we should also get the NodeCreated event
        // when setting the parent but that doesn't happen since no scene is
        // actually set on the entity and that QNodePrivate::_q_addChild will
        // return early in such a case.

        // Check that we received ComponentAdded
        for (const auto event: { entitySpy.events.takeFirst().change().dynamicCast<Qt3DCore::QComponentAddedChange>(),
                                 componentSpy.events.takeLast().change().dynamicCast<Qt3DCore::QComponentAddedChange>() })
        {
            QCOMPARE(event->type(), Qt3DCore::ComponentAdded);
            QCOMPARE(event->subjectId(), entity->id());
            QCOMPARE(event->entityId(), entity->id());
            QCOMPARE(event->componentId(), comp->id());
            QCOMPARE(event->componentMetaObject(), comp->metaObject());
        }
    }
}

void tst_Nodes::appendingComponentToEntity()
{
    // GIVEN
    ObserverSpy entitySpy;
    ObserverSpy componentSpy;
    {
        QScopedPointer<MyQEntity> entity(new MyQEntity());
        entity->setArbiterAndScene(&entitySpy);
        MyQComponent *comp = new MyQComponent(entity.data());
        comp->setArbiter(&componentSpy);
        QCoreApplication::processEvents();

        // THEN
        QVERIFY(entity->parentNode() == nullptr);
        QVERIFY(entity->children().count() == 1);
        QVERIFY(entity->components().empty());
        QVERIFY(comp->parentNode() == entity.data());

        // WHEN
        entity->addComponent(comp);

        // THEN
        QVERIFY(entity->components().count() == 1);
        QVERIFY(entity->components().first() == comp);
        QVERIFY(comp->parentNode() == entity.data());
        QCOMPARE(entitySpy.events.size(), 1);
        QVERIFY(entitySpy.events.first().wasLocked());
        for (const auto event: { entitySpy.events.takeFirst().change().dynamicCast<Qt3DCore::QComponentAddedChange>(),
             componentSpy.events.takeFirst().change().dynamicCast<Qt3DCore::QComponentAddedChange>() })
        {
            QCOMPARE(event->type(), Qt3DCore::ComponentAdded);
            QCOMPARE(event->subjectId(), entity->id());
            QCOMPARE(event->entityId(), entity->id());
            QCOMPARE(event->componentId(), comp->id());
            QCOMPARE(event->componentMetaObject(), comp->metaObject());
        }
    }
}

void tst_Nodes::removingComponentFromEntity()
{
    // GIVEN
    ObserverSpy entitySpy;
    ObserverSpy componentSpy;
    {
        QScopedPointer<MyQEntity> entity(new MyQEntity());
        entity->setArbiterAndScene(&entitySpy);
        MyQComponent *comp = new MyQComponent();
        comp->setArbiter(&componentSpy);

        // WHEN
        entity->addComponent(comp);

        // THEN
        QVERIFY(entity->components().count() == 1);
        QCOMPARE(entity->children().count(), 1);
        QVERIFY(comp->parent() == entity.data());

        // WHEN
        entitySpy.events.clear();
        componentSpy.events.clear();
        entity->removeComponent(comp);

        // THEN
        QVERIFY(entity->components().count() == 0);
        QVERIFY(comp->parent() == entity.data());
        QVERIFY(entity->children().count() == 1);
        QCOMPARE(entitySpy.events.size(), 1);
        QVERIFY(entitySpy.events.first().wasLocked());
        QCOMPARE(componentSpy.events.size(), 1);
        for (const auto event: { entitySpy.events.takeFirst().change().dynamicCast<Qt3DCore::QComponentRemovedChange>(),
                                 componentSpy.events.takeFirst().change().dynamicCast<Qt3DCore::QComponentRemovedChange>() }) {
            QCOMPARE(event->type(), Qt3DCore::ComponentRemoved);
            QCOMPARE(event->subjectId(), entity->id());
            QCOMPARE(event->entityId(), entity->id());
            QCOMPARE(event->componentId(), comp->id());
            QCOMPARE(event->componentMetaObject(), comp->metaObject());
        }
    }
}

void tst_Nodes::changeCustomProperty()
{
    // GIVEN
    ObserverSpy spy;
    QScopedPointer<MyQNode> node(new MyQNode());
    node->setArbiterAndScene(&spy);
    // WHEN
    node->setCustomProperty(QStringLiteral("foo"));
    // THEN
    QCOMPARE(spy.events.size(), 1);
    QVERIFY(spy.events.first().wasLocked());
    Qt3DCore::QPropertyUpdatedChangePtr event = spy.events.takeFirst().change().dynamicCast<Qt3DCore::QPropertyUpdatedChange>();
    QCOMPARE(event->type(), Qt3DCore::PropertyUpdated);
    QCOMPARE(event->propertyName(), "customProperty");
    QCOMPARE(event->value().toString(), QString("foo"));
}

void tst_Nodes::checkDestruction()
{
    // GIVEN
    QScopedPointer<MyQNode> root(new MyQNode());
    Qt3DCore::QEntity *entity = new Qt3DCore::QEntity(root.data());

    QCoreApplication::processEvents();

    MyQComponent *comp1 = new MyQComponent();
    MyQComponent *comp2 = new MyQComponent();
    MyQComponent *comp3 = new MyQComponent();

    entity->addComponent(comp1);
    entity->addComponent(comp2);
    entity->addComponent(comp3);

    // THEN
    QVERIFY(!root->children().isEmpty());

    // WHEN
    delete entity;

    // THEN
    QVERIFY(root->children().isEmpty());
}

void tst_Nodes::checkDefaultConstruction()
{
    // GIVEN
    Qt3DCore::QNode node;

    // THEN
    QCOMPARE(node.parentNode(), nullptr);
    QCOMPARE(node.isEnabled(), true);
    QCOMPARE(node.defaultPropertyTrackingMode(), Qt3DCore::QNode::TrackFinalValues);
}

void tst_Nodes::checkPropertyChanges()
{
    // GIVEN
    Qt3DCore::QNode parentNode;
    Qt3DCore::QNode node;

    {
        // WHEN
        QSignalSpy spy(&node, SIGNAL(parentChanged(QObject *)));
        Qt3DCore::QNode *newValue = &parentNode;
        node.setParent(newValue);

        // THEN
        QVERIFY(spy.isValid());
        QCOMPARE(node.parentNode(), newValue);
        QCOMPARE(spy.count(), 1);

        // WHEN
        spy.clear();
        node.setParent(newValue);

        // THEN
        QCOMPARE(node.parentNode(), newValue);
        QCOMPARE(spy.count(), 0);
    }
    {
        // WHEN
        QSignalSpy spy(&node, SIGNAL(enabledChanged(bool)));
        const bool newValue = false;
        node.setEnabled(newValue);

        // THEN
        QVERIFY(spy.isValid());
        QCOMPARE(node.isEnabled(), newValue);
        QCOMPARE(spy.count(), 1);

        // WHEN
        spy.clear();
        node.setEnabled(newValue);

        // THEN
        QCOMPARE(node.isEnabled(), newValue);
        QCOMPARE(spy.count(), 0);
    }
    {
        // WHEN
        QSignalSpy spy(&node, SIGNAL(defaultPropertyTrackingModeChanged(PropertyTrackingMode)));
        const Qt3DCore::QNode::PropertyTrackingMode newValue = Qt3DCore::QNode::TrackAllValues;
        node.setDefaultPropertyTrackingMode(newValue);

        // THEN
        QVERIFY(spy.isValid());
        QCOMPARE(node.defaultPropertyTrackingMode(), newValue);
        QCOMPARE(spy.count(), 1);

        // WHEN
        spy.clear();
        node.setDefaultPropertyTrackingMode(newValue);

        // THEN
        QCOMPARE(node.defaultPropertyTrackingMode(), newValue);
        QCOMPARE(spy.count(), 0);
    }
    {
        // WHEN
        const QString enabledPropertyName = QStringLiteral("enabled");
        node.setDefaultPropertyTrackingMode(Qt3DCore::QNode::DontTrackValues);
        node.setPropertyTracking(enabledPropertyName, Qt3DCore::QNode::TrackAllValues);

        // THEN
        QCOMPARE(node.propertyTracking(enabledPropertyName), Qt3DCore::QNode::TrackAllValues);

        // WHEN
        node.clearPropertyTracking(enabledPropertyName);

        // THEN
        QCOMPARE(node.propertyTracking(enabledPropertyName), Qt3DCore::QNode::DontTrackValues);
    }
}

void tst_Nodes::checkCreationData()
{
    // GIVEN
    Qt3DCore::QNode root;
    Qt3DCore::QNode node;

    node.setParent(&root);
    node.setEnabled(true);
    const QString enabledPropertyName = QStringLiteral("enabled");
    node.setDefaultPropertyTrackingMode(Qt3DCore::QNode::DontTrackValues);
    node.setPropertyTracking(enabledPropertyName, Qt3DCore::QNode::TrackAllValues);

    // WHEN
    QVector<Qt3DCore::QNodeCreatedChangeBasePtr> creationChanges;

    {
        Qt3DCore::QNodeCreatedChangeGenerator creationChangeGenerator(&node);
        creationChanges = creationChangeGenerator.creationChanges();
    }

    // THEN
    {
        QCOMPARE(creationChanges.size(), 1);

        const auto creationChangeData = qSharedPointerCast<Qt3DCore::QNodeCreatedChangeBase>(creationChanges.first());

        QCOMPARE(node.id(), creationChangeData->subjectId());
        QCOMPARE(node.isEnabled(), true);
        QCOMPARE(node.isEnabled(), creationChangeData->isNodeEnabled());
        QCOMPARE(node.metaObject(), creationChangeData->metaObject());
    }

    // WHEN
    node.setEnabled(false);

    {
        Qt3DCore::QNodeCreatedChangeGenerator creationChangeGenerator(&node);
        creationChanges = creationChangeGenerator.creationChanges();
    }

    // THEN
    {
        QCOMPARE(creationChanges.size(), 1);

        const auto creationChangeData = qSharedPointerCast<Qt3DCore::QNodeCreatedChangeBase>(creationChanges.first());

        QCOMPARE(node.id(), creationChangeData->subjectId());
        QCOMPARE(node.isEnabled(), false);
        QCOMPARE(node.isEnabled(), creationChangeData->isNodeEnabled());
        QCOMPARE(node.metaObject(), creationChangeData->metaObject());
    }
}

void tst_Nodes::checkEnabledUpdate()
{
    // GIVEN
    TestArbiter arbiter;
    Qt3DCore::QNode node;
    arbiter.setArbiterOnNode(&node);

    {
        // WHEN
        node.setEnabled(false);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        auto change = arbiter.events.first().staticCast<Qt3DCore::QPropertyUpdatedChange>();
        QCOMPARE(change->propertyName(), "enabled");
        QCOMPARE(change->value().value<bool>(), node.isEnabled());
        QCOMPARE(change->type(), Qt3DCore::PropertyUpdated);

        arbiter.events.clear();
    }

    {
        // WHEN
        node.setEnabled(false);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 0);
    }

}

void tst_Nodes::checkPropertyTrackModeUpdate()
{
    // GIVEN
    TestArbiter arbiter;
    Qt3DCore::QNode node;
    arbiter.setArbiterOnNode(&node);

    {
        // WHEN
        node.setDefaultPropertyTrackingMode(Qt3DCore::QNode::TrackAllValues);
        QCoreApplication::processEvents();

        // THEN -> this properties is non notifying
        QCOMPARE(arbiter.events.size(), 0);
    }

    {
        // WHEN
        node.setDefaultPropertyTrackingMode(Qt3DCore::QNode::TrackAllValues);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 0);
    }

}

void tst_Nodes::checkTrackedPropertyNamesUpdate()
{
    // GIVEN
    TestArbiter arbiter;
    Qt3DCore::QNode node;
    arbiter.setArbiterOnNode(&node);

    {
        // WHEN
        node.setPropertyTracking(QStringLiteral("883"), Qt3DCore::QNode::TrackAllValues);
        QCoreApplication::processEvents();

        // THEN -> this properties is non notifying
        QCOMPARE(arbiter.events.size(), 0);
    }

    {
        // WHEN
        node.setPropertyTracking(QStringLiteral("883"), Qt3DCore::QNode::DontTrackValues);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 0);
    }

}

QTEST_MAIN(tst_Nodes)

#include "tst_nodes.moc"
