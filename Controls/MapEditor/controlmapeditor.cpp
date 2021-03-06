﻿/*
    RPG Paper Maker Copyright (C) 2017 Marie Laporte

    This file is part of RPG Paper Maker.

    RPG Paper Maker is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPG Paper Maker is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "controlmapeditor.h"
#include "dialogobject.h"
#include "wanok.h"
#include <QTime>
#include <QApplication>

// -------------------------------------------------------

#include "controlmapeditor-raycasting.cpp"
#include "controlmapeditor-preview.cpp"
#include "controlmapeditor-add-remove.cpp"
#include "controlmapeditor-objects.cpp"

// -------------------------------------------------------
//
//  CONSTRUCTOR / DESTRUCTOR / GET / SET
//
// -------------------------------------------------------

ControlMapEditor::ControlMapEditor() :
    m_map(nullptr),
    m_grid(nullptr),
    m_beginWallIndicator(nullptr),
    m_endWallIndicator(nullptr),
    m_cursorObject(nullptr),
    m_camera(new Camera),
    m_elementOnLand(nullptr),
    m_elementOnSprite(nullptr),
    m_positionPreviousPreview(-1, 0, 0, -1, 0),
    m_previousMouseCoords(-500, 0, 0, -500),
    m_needMapInfosToSave(false),
    m_needMapObjectsUpdate(false),
    m_displayGrid(true),
    m_displaySquareInformations(true),
    m_treeMapNode(nullptr),
    m_isDrawingWall(false),
    m_isDeletingWall(false),
    m_isDeleting(false),
    m_isCtrlPressed(false),
    m_isMovingObject(false)
{

}

ControlMapEditor::~ControlMapEditor(){
    deleteMap(false);
    delete m_camera;
}

Map* ControlMapEditor::map() const { return m_map; }

Grid* ControlMapEditor::grid() const { return m_grid; }

Cursor* ControlMapEditor::cursor() const { return m_map->cursor(); }

Cursor* ControlMapEditor::cursorObject() const { return m_cursorObject; }

Camera* ControlMapEditor::camera() const { return m_camera; }

bool ControlMapEditor::isCtrlPressed() const { return m_isCtrlPressed; }

void ControlMapEditor::setIsCtrlPressed(bool b) { m_isCtrlPressed = b; }

bool ControlMapEditor::displaySquareInformations() const {
    return m_displaySquareInformations;
}

void ControlMapEditor::setContextMenu(ContextMenuList* m){
    m_contextMenu = m;
}

void ControlMapEditor::setTreeMapNode(QStandardItem* item) {
    m_treeMapNode = item;
}

// -------------------------------------------------------
//
//  INTERMEDIARY FUNCTIONS
//
// -------------------------------------------------------

void ControlMapEditor::moveCursorToMousePosition(QPoint point)
{
    updateMousePosition(point);

    if (m_map->isInGrid(m_positionOnPlane))
        cursor()->setPositions(m_positionOnPlane);
}

// -------------------------------------------------------

void ControlMapEditor::reLoadTextures(){
    m_map->loadTextures();
}

// -------------------------------------------------------

Map* ControlMapEditor::loadMap(int idMap, QVector3D* position,
                               QVector3D *positionObject, int cameraDistance,
                               double cameraHorizontalAngle,
                               double cameraVerticalAngle)
{
    clearPortionsToUpdate();

    // Map & cursor
    m_map = new Map(idMap);
    Wanok::get()->project()->setCurrentMap(m_map);
    m_map->initializeCursor(position);
    m_map->initializeGL();
    // Update current portion and load all the local portions
    m_currentPortion = cursor()->getPortion();
    m_map->loadPortions(m_currentPortion);

    // Grid
    m_grid = new Grid;
    m_grid->initializeVertices(m_map->mapProperties()->length(),
                               m_map->mapProperties()->width(),
                               m_map->squareSize());
    m_grid->initializeGL();

    // Cursor object
    m_cursorObject = new Cursor(positionObject);
    m_cursorObject->initializeSquareSize(m_map->squareSize());
    Position pos(m_cursorObject->getSquareX(),
                 m_cursorObject->getSquareY(),
                 0,
                 m_cursorObject->getSquareZ(),
                 0);
    setCursorObjectPosition(pos);
    m_cursorObject->setFrameNumber(1);
    m_cursorObject->loadTexture(
                ":/textures/Ressources/object_square_cursor.png");
    m_cursorObject->initialize();

    // Wall indicator
    m_beginWallIndicator = new WallIndicator;
    m_endWallIndicator = new WallIndicator;
    updateWallIndicator();
    m_beginWallIndicator->initializeSquareSize(m_map->squareSize());
    m_beginWallIndicator->initializeVertices();
    m_beginWallIndicator->initializeGL();
    m_endWallIndicator->initializeSquareSize(m_map->squareSize());
    m_endWallIndicator->initializeVertices();
    m_endWallIndicator->initializeGL();

    // Camera
    m_camera->setDistance(cameraDistance * Wanok::coefSquareSize());
    m_camera->setHorizontalAngle(cameraHorizontalAngle);
    m_camera->setVerticalAngle(cameraVerticalAngle);
    m_camera->update(cursor(), m_map->squareSize());

    return m_map;
}

// -------------------------------------------------------

void ControlMapEditor::deleteMap(bool updateCamera){
    clearPortionsToUpdate();
    removePreviewElements();

    // Cursors
    if (m_cursorObject != nullptr){
        delete m_cursorObject;
        m_cursorObject = nullptr;
    }
    if (m_beginWallIndicator != nullptr){
        delete m_beginWallIndicator;
        m_beginWallIndicator = nullptr;
    }
    if (m_endWallIndicator != nullptr){
        delete m_endWallIndicator;
        m_endWallIndicator = nullptr;
    }

    // Grid
    if (m_grid != nullptr){
        delete m_grid;
        m_grid = nullptr;
    }

    // Map
    if (m_map != nullptr){
        delete m_map;
        m_map = nullptr;
    }

    // Update camera node
    if (updateCamera && m_treeMapNode != nullptr) {
        m_camera->setDistance(m_camera->distance() / Wanok::coefSquareSize());
        updateCameraTreeNode();
    }
}

// -------------------------------------------------------

void ControlMapEditor::onResize(int width, int height){
    m_width = width;
    m_height = height;
    m_camera->setProjection(width, height);
}

// -------------------------------------------------------

void ControlMapEditor::updateCameraTreeNode(){
    TreeMapTag* tag = (TreeMapTag*) m_treeMapNode->data().value<quintptr>();
    tag->setCameraDistance(m_camera->distance());
    tag->setCameraHorizontalAngle(m_camera->horizontalAngle());
    tag->setCameraVerticalAngle(m_camera->verticalAngle());
}

// -------------------------------------------------------
//
//  UPDATES
//
// -------------------------------------------------------

void ControlMapEditor::update(bool layerOn)
{
    // Update portions
    updatePortions();
    saveTempPortions();
    clearPortionsToUpdate();
    updateMovingPortions();

    // Camera
    m_camera->update(cursor(), m_map->squareSize());

    // Raycasting
    updateRaycasting(layerOn);

    // Mouse update
    m_mouseBeforeUpdate = m_mouseMove;
}

// -------------------------------------------------------

void ControlMapEditor::updateMouse(QPoint point, bool layerOn) {
    updateMousePosition(point);
    m_mouseMove = point;
    updateRaycasting(layerOn);
    m_mouseBeforeUpdate = m_mouseMove;
}

// -------------------------------------------------------

void ControlMapEditor::updateMousePosition(QPoint point) {
    m_mouse = point;
}

// -------------------------------------------------------

void ControlMapEditor::updateMouseMove(QPoint point) {
    updateMousePosition(point);
    m_mouseMove = point;
}

// -------------------------------------------------------

bool ControlMapEditor::mousePositionChanged(QPoint point) {
    return m_mouse != point;
}

// -------------------------------------------------------

void ControlMapEditor::updateWallIndicator() {
    if (!m_isDrawingWall && !m_isDeletingWall) {
        m_beginWallIndicator->setPosition(m_positionOnPlaneWallIndicator,
                                          m_map->mapProperties()->length(),
                                          m_map->mapProperties()->width());
    }
    m_endWallIndicator->setPosition(m_positionOnPlaneWallIndicator,
                                    m_map->mapProperties()->length(),
                                    m_map->mapProperties()->width());
}

// -------------------------------------------------------

void ControlMapEditor::updatePortions() {
    if (m_needMapObjectsUpdate) {
        m_needMapObjectsUpdate = false;
        m_map->updateMapObjects();
    }

    QSet<MapPortion*>::iterator i;
    for (i = m_portionsToUpdate.begin(); i != m_portionsToUpdate.end(); i++) {
        MapPortion* mapPortion = *i;
        m_map->updatePortion(mapPortion);
    }
}

// -------------------------------------------------------

void ControlMapEditor::updateMovingPortions() {

    // Move portions
    Portion newPortion = cursor()->getPortion();
    if (qAbs(m_currentPortion.x() - newPortion.x()) < m_map->getMapPortionSize()
        && qAbs(m_currentPortion.z() - newPortion.z()) <
        m_map->getMapPortionSize())
    {
        updateMovingPortionsEastWest(newPortion);
        updateMovingPortionsNorthSouth(newPortion);
        updateMovingPortionsUpDown(newPortion);
    }
    else {
        m_map->loadPortions(newPortion);
        m_currentPortion = newPortion;
    }
}

// -------------------------------------------------------

void ControlMapEditor::updateMovingPortionsEastWest(Portion& newPortion){
    int r = m_map->portionsRay();
    if (newPortion.x() > m_currentPortion.x()) {
        int dif = newPortion.x() - m_currentPortion.x();
        int state = 1;
        while (state <= dif) {
            int k = 0;
            for (int j = -r; j <= r; j++) {
                bool visible = j != -r && j != r;
                int i = -r;
                removePortion(i, k, j);
                setPortion(i, k, j, i + 1, k, j, false);
                i++;
                for (; i < r; i++)
                    setPortion(i, k, j, i + 1, k, j, visible);

                loadPortion(m_currentPortion.x() + state,
                            m_currentPortion.y(), m_currentPortion.z(),
                            r, k, j);
            }
            state++;
        }
        m_currentPortion.setX(m_currentPortion.x() + dif);
    }
    else if (newPortion.x() < m_currentPortion.x()) {
        int dif = m_currentPortion.x() - newPortion.x();
        int state = 1;
        while (state <= dif) {
            int k = 0;
            for (int j = -r; j <= r; j++) {
                bool visible = j != -r && j != r;
                int i = r;
                removePortion(i, k, j);
                setPortion(i, k, j, i - 1, k, j, false);
                i--;
                for (; i > -r; i--)
                    setPortion(i, k, j, i - 1, k, j, visible);

                loadPortion(m_currentPortion.x() - state,
                            m_currentPortion.y(), m_currentPortion.z(),
                            -r, k, j);
            }
            state++;
        }
        m_currentPortion.setX(m_currentPortion.x() - dif);
    }
}

// -------------------------------------------------------

void ControlMapEditor::updateMovingPortionsNorthSouth(Portion& newPortion){
    int r = m_map->portionsRay();
    if (newPortion.z() > m_currentPortion.z()) {
        int dif = newPortion.z() - m_currentPortion.z();
        int state = 1;
        while (state <= dif) {
            int k = 0;
            for (int i = -r; i <= r; i++) {
                bool visible = i != -r && i != r;
                int j = -r;
                removePortion(i, k, j);
                setPortion(i, k, j, i, k, j + 1, false);
                j++;
                for (; j < r; j++)
                    setPortion(i, k, j, i, k, j + 1, visible);

                loadPortion(m_currentPortion.x(), m_currentPortion.y(),
                            m_currentPortion.z() + state, i, k, r);
            }
            state++;
        }
        m_currentPortion.setZ(m_currentPortion.z() + dif);
    }
    else if (newPortion.z() < m_currentPortion.z()) {
        int dif = m_currentPortion.z() - newPortion.z();
        int state = 1;
        while (state <= dif) {
            int k = 0;
            for (int i = -r; i <= r; i++) {
                bool visible = i != -r && i != r;
                int j = r;
                removePortion(i, k, j);
                setPortion(i, k, j, i, k, j - 1, false);
                j--;
                for (; j > -r; j--)
                    setPortion(i, k, j, i, k, j - 1, visible);

                loadPortion(m_currentPortion.x(), m_currentPortion.y(),
                            m_currentPortion.z() - state, i, k, -r);
            }
            state++;
        }
        m_currentPortion.setZ(m_currentPortion.z() - dif);
    }
}

// -------------------------------------------------------

void ControlMapEditor::updateMovingPortionsUpDown(Portion&){
    // TODO
}

// -------------------------------------------------------

void ControlMapEditor::removePortion(int i, int j, int k){
    MapPortion* mapPortion = m_map->mapPortion(i, j, k);
    if (mapPortion != nullptr)
        delete mapPortion;
}

// -------------------------------------------------------

void ControlMapEditor::setPortion(int i, int j, int k, int m, int n, int o,
                                  bool visible)
{
    Portion previousPortion(i, j, k);
    Portion newPortion(m, n, o);

    m_map->replacePortion(previousPortion, newPortion, visible);
}

// -------------------------------------------------------

void ControlMapEditor::loadPortion(int a, int b, int c, int i, int j, int k)
{
    m_map->loadPortion(a + i, b + j, c + k, i, j, k, false);
}

// -------------------------------------------------------

void ControlMapEditor::saveTempPortions(){
    QSet<MapPortion*>::iterator i;
    for (i = m_portionsToSave.begin(); i != m_portionsToSave.end(); i++)
        m_map->savePortionMap(*i);

    QHash<Portion, MapPortion*>::iterator j;
    for (j = m_portionsGlobalSave.begin(); j != m_portionsGlobalSave.end(); j++)
        m_map->savePortionMap(*j);

    // Save file infos
    if (m_needMapInfosToSave) {
        m_map->saveMapProperties();
        m_needMapInfosToSave = false;
    }
}

// -------------------------------------------------------

void ControlMapEditor::clearPortionsToUpdate(){
    m_portionsToUpdate.clear();
    m_portionsToSave.clear();

    QHash<Portion, MapPortion*>::iterator i;
    for (i = m_portionsGlobalSave.begin(); i != m_portionsGlobalSave.end(); i++)
        delete *i;

    m_portionsGlobalSave.clear();
}

// -------------------------------------------------------

void ControlMapEditor::setToNotSaved(){
    m_map->setSaved(false);
    Wanok::mapsToSave.insert(m_map->mapProperties()->id());
    m_treeMapNode->setText(m_map->mapProperties()->name() + " *");
}

// -------------------------------------------------------

void ControlMapEditor::save(){
    m_treeMapNode->setText(m_map->mapProperties()->name());
}

// -------------------------------------------------------

MapElement* ControlMapEditor::getPositionSelected(
        Position& position, MapEditorSelectionKind selection,
        MapEditorSubSelectionKind subSelection, bool layerOn,
        bool isForDisplay) const
{
    switch (selection){
    case MapEditorSelectionKind::Land:
        position = m_positionOnLand;
        return m_elementOnLand;
    case MapEditorSelectionKind::Sprites:
        if ((m_isDeleting && subSelection !=
             MapEditorSubSelectionKind::SpritesWall) || layerOn || isForDisplay)
        {
            position = m_positionOnSprite;
            return m_elementOnSprite;
        }

        position = m_positionOnPlane;
        return nullptr;
    case MapEditorSelectionKind::Objects:
        position = m_positionOnPlane;
        return nullptr;
    default:
        position = m_positionOnPlane;
        return nullptr;
    }
}

// -------------------------------------------------------

void ControlMapEditor::getWallSpritesPositions(QList<Position> &positions) {
    int x, y, yPlus, z;
    Position3D begin, end;
    m_beginWallIndicator->getPosition(begin);
    m_endWallIndicator->getPosition(end);

    // Can't build a wall if not in the same height
    if (begin.y() != end.y())
        return;
    y = begin.y(), yPlus = begin.yPlus();

    // Vertical
    if (begin.x() == end.x()) {
        x = begin.x();
        int upZ = qMin(begin.z(), end.z());
        int downZ = qMax(begin.z(), end.z());

        for (int i = upZ; i < downZ; i++)
            positions.append(Position(x, y, yPlus, i, 0, 0, 50, 90));
    }

    // Horizontal
    else if (begin.z() == end.z()) {
        z = begin.z();
        int leftX = qMin(begin.x(), end.x());
        int rightX = qMax(begin.x(), end.x());

        for (int i = leftX; i < rightX; i++)
            positions.append(Position(i, y, yPlus, z, 0, 50, 0, 0));
    }
}

// -------------------------------------------------------

LandDatas* ControlMapEditor::getLand(Portion& portion, Position& p){
    MapPortion* mapPortion = m_map->mapPortion(portion);
    if (mapPortion == nullptr)
        return nullptr;
    return mapPortion->getLand(p);
}

// -------------------------------------------------------

void ControlMapEditor::getFloorTextureReduced(QRect& rect, QRect &rectAfter,
                                              int localX, int localZ)
{
    rectAfter.setX(rect.x() + Wanok::mod(localX, rect.width()));
    rectAfter.setY(rect.y() + Wanok::mod(localZ, rect.height()));
    rectAfter.setWidth(1);
    rectAfter.setHeight(1);
}

// -------------------------------------------------------

bool ControlMapEditor::areLandsEquals(LandDatas* landBefore,
                                      QRect& textureAfter,
                                      MapEditorSubSelectionKind kindAfter)
{
    if (landBefore == nullptr)
        return kindAfter == MapEditorSubSelectionKind::None;
    else
        return (landBefore->getSubKind() == kindAfter)
                ? (*landBefore->textureRect()) == textureAfter : false;
}

// -------------------------------------------------------

LandDatas* ControlMapEditor::getLandAfter(MapEditorSubSelectionKind kindAfter,
                                          int specialID, QRect &textureAfter,
                                          bool up)
{
    QRect* texture = new QRect(textureAfter.x(),
                               textureAfter.y(),
                               textureAfter.width(),
                               textureAfter.height());
    switch (kindAfter) {
    case MapEditorSubSelectionKind::Floors:
        return new FloorDatas(texture, up);
    case MapEditorSubSelectionKind::Autotiles:
        return new AutotileDatas(specialID, texture, up);
    case MapEditorSubSelectionKind::Water:
        return nullptr;
    default:
        return nullptr;
    }
}

// -------------------------------------------------------

void ControlMapEditor::getLandTexture(QRect& rect, LandDatas* land){
    rect.setX(land->textureRect()->x());
    rect.setY(land->textureRect()->y());
    rect.setWidth(land->textureRect()->width());
    rect.setHeight(land->textureRect()->height());
}

// -------------------------------------------------------

SpriteDatas* ControlMapEditor::getCompleteSprite(MapEditorSubSelectionKind kind,
        int xOffset, int yOffset, int zOffset, QRect& tileset, bool front,
        bool layerOn) const
{
    SpriteDatas* sprite = new SpriteDatas(kind, new QRect(tileset), front);
    if (layerOn) {
        sprite->setXOffset(xOffset);
        sprite->setYOffset(yOffset);
        sprite->setZOffset(zOffset);
    }

    return sprite;
}

// -------------------------------------------------------

void ControlMapEditor::updatePortionsToSaveOverflow(
        QSet<Portion>& portionsOverflow)
{
    for (QSet<Portion>::const_iterator i = portionsOverflow.begin();
         i != portionsOverflow.end(); i++)
    {
        Portion portion = *i;
        MapPortion* newMapPortion = m_map->mapPortionFromGlobal(portion);

        if (newMapPortion != nullptr)
            m_portionsToSave += newMapPortion;
    }

}

// -------------------------------------------------------

MapPortion* ControlMapEditor::getMapPortion(Position& p, Portion& portion,
                                            bool undoRedo)
{
    MapPortion* mapPortion = nullptr;
    m_map->getLocalPortion(p, portion);

    if (m_map->isInPortion(portion, undoRedo ? 0 : -1))
        mapPortion = m_map->mapPortion(portion);
    else if (undoRedo) {
        Portion globalPortion;
        m_map->getGlobalPortion(p, globalPortion);
        mapPortion = m_portionsGlobalSave.value(globalPortion);

        if (mapPortion == nullptr) {
            mapPortion = m_map->loadPortionMap(globalPortion.x(),
                                               globalPortion.y(),
                                               globalPortion.z(), true);
            m_portionsGlobalSave.insert(globalPortion, mapPortion);
        }
    }

    return mapPortion;
}

// -------------------------------------------------------

void ControlMapEditor::traceLine(Position& previousCoords, Position& coords,
                                 QList<Position>& positions)
{
    if (m_previousMouseCoords.x() == -1)
        return;

    int x1 = previousCoords.x(), x2 = coords.x();
    int y = coords.y();
    int yPlus = coords.yPlus();
    int z1 = previousCoords.z(), z2 = coords.z();
    int l = coords.layer();
    int dx = x2 - x1, dz = z2 - z1;
    bool test = true;

    if (dx != 0){
        if (dx > 0){
            if (dz != 0){
                if (dz > 0){
                    if (dx >= dz){
                        int e = dx;
                        dx = 2 * e;
                        dz = dz * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            x1++;
                            if (x1 == x2) break;
                            e -= dz;
                            if (e < 0){
                                z1++;
                                e += dx;
                            }
                        }
                    }
                    else{
                        int e = dz;
                        dz = 2 * e;
                        dx = dx * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            z1++;
                            if (z1 == z2) break;
                            e -= dx;
                            if (e < 0){
                                x1++;
                                e += dz;
                            }
                        }
                    }
                }
                else{
                    if (dx >= -dz){
                        int e = dx;
                        dx = 2 * e;
                        dz = dz * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            x1++;
                            if (x1 == x2) break;
                            e += dz;
                            if (e < 0){
                                z1--;
                                e += dx;
                            }
                        }
                    }
                    else{
                        int e = dz;
                        dz = 2 * e;
                        dx = dx * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            z1--;
                            if (z1 == z2) break;
                            e += dx;
                            if (e > 0){
                                x1++;
                                e += dz;
                            }
                        }
                    }
                }
            }
            else{
                while (x1 != x2){
                    positions.push_back(Position(x1, y, yPlus, z1, l));
                    x1++;
                }
            }
        }
        else{
            dz = z2 - z1;
            if (dz != 0){
                if (dz > 0){
                    if (-dx >= dz){
                        int e = dx;
                        dx = 2 * e;
                        dz = dz * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            x1--;
                            if (x1 == x2) break;
                            e += dz;
                            if (e >= 0){
                                z1++;
                                e += dx;
                            }
                        }
                    }
                    else{
                        int e = dz;
                        dz = 2 * e;
                        dx = dx * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            z1++;
                            if (z1 == z2) break;
                            e += dx;
                            if (e <= 0)
                            {
                                x1--;
                                e += dz;
                            }
                        }
                    }
                }
                else{
                    if (dx <= dz){
                        int e = dx;
                        dx = 2 * e;
                        dz = dz * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            x1--;
                            if (x1 == x2) break;
                            e -= dz;
                            if (e >= 0){
                                z1--;
                                e += dx;
                            }
                        }
                    }
                    else{
                        int e = dz;
                        dz = 2 * e;
                        dx = dx * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            z1--;
                            if (z1 == z2) break;
                            e -= dx;
                            if (e >= 0){
                                x1--;
                                e += dz;
                            }
                        }
                    }
                }
            }
            else{
                while (x1 != x2){
                    positions.push_back(Position(x1, y, yPlus, z1, l));
                    x1--;
                }
            }
        }
    }
    else{
        dz = z2 - z1;
        if (dz != 0){
            if (dz > 0){
                while(z1 != z2){
                    positions.push_back(Position(x1, y, yPlus, z1, l));
                    z1++;
                }
            }
            else{
                while (z1 != z2){
                    positions.push_back(Position(x1, y, yPlus, z1, l));
                    z1--;
                }
            }
        }
    }
}

// -------------------------------------------------------

int ControlMapEditor::getLayer(MapPortion *mapPortion, float d, Position& p,
                               bool layerOn, MapEditorSelectionKind kind)
{
    if (m_currentLayer == -1) {
        if (d != 0) {
            int layer = p.layer();
            if (layerOn)
                layer = mapPortion->getLastLayerAt(p, kind) + 1;

            return layer;
        }
        return 0;
    }

    return m_currentLayer;
}

// -------------------------------------------------------

void ControlMapEditor::undo() {
    QJsonArray states;
    m_controlUndoRedo.undo(m_map->mapProperties()->id(), states);
    undoRedo(states, true);
}

// -------------------------------------------------------

void ControlMapEditor::redo() {
    QJsonArray states;
    m_controlUndoRedo.redo(m_map->mapProperties()->id(), states);
    undoRedo(states, false);
}

// -------------------------------------------------------

void ControlMapEditor::undoRedo(QJsonArray& states, bool reverseAction) {
    for (int i = 0; i < states.size(); i++) {
        QJsonObject objState = states.at(i).toObject(), objBefore, objAfter;
        MapEditorSubSelectionKind kindBefore, kindAfter;
        Position position;
        m_controlUndoRedo.getStateInfos(objState, kindBefore, kindAfter,
                                        objBefore, objAfter, position);
        if (reverseAction) {
            performUndoRedoAction(kindAfter, !reverseAction, objAfter,
                                  position);
            performUndoRedoAction(kindBefore, reverseAction, objBefore,
                                  position);
        }
        else {
            performUndoRedoAction(kindBefore, reverseAction, objBefore,
                                  position);
            performUndoRedoAction(kindAfter, !reverseAction, objAfter,
                                  position);
        }
    }
}

// -------------------------------------------------------

void ControlMapEditor::performUndoRedoAction(
        MapEditorSubSelectionKind kind, bool before, QJsonObject& obj,
        Position& position)
{
    switch (kind) {
    case MapEditorSubSelectionKind::None:
        break;
    case MapEditorSubSelectionKind::Floors:
    case MapEditorSubSelectionKind::Autotiles:
    {
        if (before) {
            LandDatas* land = nullptr;
            switch (kind) {
            case MapEditorSubSelectionKind::Floors:
                land = new FloorDatas; break;
            case MapEditorSubSelectionKind::Autotiles:
                land = new AutotileDatas; break;
            default:
                break;
            }
            land->read(obj);
            stockLand(position, land, kind, false, true);
        }
        else
            eraseLand(position, true);
        break;
    }
    case MapEditorSubSelectionKind::SpritesFace:
    case MapEditorSubSelectionKind::SpritesFix:
    case MapEditorSubSelectionKind::SpritesDouble:
    case MapEditorSubSelectionKind::SpritesQuadra:
    {
        if (before) {
            SpriteDatas* sprite = new SpriteDatas;
            sprite->read(obj);
            stockSprite(position, sprite, kind, false, true);
        }
        else
            eraseSprite(position, true);
        break;
    }
    case MapEditorSubSelectionKind::SpritesWall:
    {
        if (before) {
            SpriteWallDatas* sprite = new SpriteWallDatas;
            sprite->read(obj);
            stockSpriteWall(position, sprite, true);
        }
        else
            eraseSpriteWall(position, true);
        break;
    }
    case MapEditorSubSelectionKind::Object:
        if (before) {
            SystemCommonObject* object = new SystemCommonObject;
            object->read(obj);
            stockObject(position, object, true);
        }
        else
            eraseObject(position, true);
        break;
    default:
        break;
    }
}

// -------------------------------------------------------
//
//  GL
//
// -------------------------------------------------------

void ControlMapEditor::paintGL(QMatrix4x4 &modelviewProjection,
                               QVector3D &cameraRightWorldSpace,
                               QVector3D &cameraUpWorldSpace,
                               QVector3D &cameraDeepWorldSpace,
                               MapEditorSelectionKind selectionKind,
                               MapEditorSubSelectionKind subSelectionKind,
                               DrawKind drawKind)
{
    // Drawing floors
    m_map->paintFloors(modelviewProjection);

    // Drawing object cursor
    if (selectionKind == MapEditorSelectionKind::Objects) {
        if (isCursorObjectVisible())
            m_cursorObject->paintGL(modelviewProjection);
    }

    // Drawing user cursor
    m_map->cursor()->paintGL(modelviewProjection);

    // Drawing grid
    if (m_displayGrid){
        m_grid->paintGL(modelviewProjection);
    }

    // Drawing other stuff
    m_map->paintOthers(modelviewProjection, cameraRightWorldSpace,
                       cameraUpWorldSpace, cameraDeepWorldSpace);

    // Drawing wall indicator
    if (subSelectionKind == MapEditorSubSelectionKind::SpritesWall &&
        drawKind != DrawKind::Pin)
    {
        m_beginWallIndicator->paintGL(modelviewProjection);
        m_endWallIndicator->paintGL(modelviewProjection);
    }
}

// -------------------------------------------------------

void ControlMapEditor::showHideGrid() {
    m_displayGrid = !m_displayGrid;
}

// -------------------------------------------------------

void ControlMapEditor::showHideSquareInformations() {
    m_displaySquareInformations = !m_displaySquareInformations;
}

// -------------------------------------------------------

QString ControlMapEditor::getSquareInfos(MapEditorSelectionKind kind,
                                         MapEditorSubSelectionKind subKind,
                                         bool layerOn, bool focus)
{
    if (focus) {
        Position position;
        MapElement* element = getPositionSelected(position, kind, subKind,
                                                  layerOn, true);
        if (!m_map->isInGrid(position))
            m_lastSquareInfos = "";
        else {
            m_lastSquareInfos =
               (element == nullptr ? "[NONE]" : "[" + element->toString() + "]")
               + "\n" + position.toString(m_map->squareSize());
        }
    }

    return m_lastSquareInfos;
}

// -------------------------------------------------------

bool ControlMapEditor::isVisible(Position3D& position) {
    Portion portion;
    m_map->getLocalPortion(position, portion);

    return m_map->isInPortion(portion);
}

// -------------------------------------------------------
//
//  EVENTS
//
// -------------------------------------------------------

void ControlMapEditor::onMouseWheelMove(QWheelEvent* event, bool updateTree){
    if (event->delta() > 0)
        m_camera->zoomPlus(m_map->squareSize());
    else
        m_camera->zoomLess(m_map->squareSize());

    if (updateTree)
        updateCameraTreeNode();
}

// -------------------------------------------------------

void ControlMapEditor::onMouseMove(QPoint point,
                                   Qt::MouseButton button, bool updateTree)
{
    updateMousePosition(point);
    m_mouseMove = point;

    if (button == Qt::MouseButton::MiddleButton) {
        m_camera->onMouseWheelPressed(m_mouseMove, m_mouseBeforeUpdate);
        if (updateTree)
            updateCameraTreeNode();
    }
}

// -------------------------------------------------------

void ControlMapEditor::onMousePressed(MapEditorSelectionKind selection,
                                      MapEditorSubSelectionKind subSelection,
                                      DrawKind drawKind, bool layerOn,
                                      QRect &tileset, int specialID,
                                      QPoint point, Qt::MouseButton button)
{

    // Update mouse
    updateMouse(point, layerOn);

    if (button != Qt::MouseButton::MiddleButton) {

        // If ctrl key is pressed, teleport
        if (m_isCtrlPressed) {
            moveCursorToMousePosition(point);
            return;
        }

        // Add/Remove something
        m_isDeleting = button == Qt::MouseButton::RightButton;
        if (m_isDeleting)
            removePreviewElements();
        Position newPosition;
        getPositionSelected(newPosition, selection, subSelection, layerOn);
        if (((Position3D) m_previousMouseCoords) != ((Position3D) newPosition))
        {
            m_previousMouseCoords = newPosition;
            addRemove(selection, subSelection, drawKind, layerOn, tileset,
                      specialID);

            // Wall sprite
            if (subSelection == MapEditorSubSelectionKind::SpritesWall)
            {
                if (button == Qt::MouseButton::LeftButton)
                    m_isDrawingWall = true;
                else if (button == Qt::MouseButton::RightButton)
                    m_isDeletingWall = true;
            }
        }
    }
}

// -------------------------------------------------------

void ControlMapEditor::onMouseReleased(MapEditorSelectionKind,
                                       MapEditorSubSelectionKind,
                                       DrawKind drawKind,
                                       QRect &, int specialID,
                                       QPoint,
                                       Qt::MouseButton button)
{
    if (button == Qt::MouseButton::LeftButton) {
        if (m_isDrawingWall) {
            m_isDrawingWall = false;
            addSpriteWall(drawKind, specialID);
        }
    }
    else if (button == Qt::MouseButton::RightButton) {
        if (m_isDeletingWall) {
            m_isDeletingWall = false;
            removeSpriteWall(drawKind);
        }
    }

    // Force previous mouse coords to be different
    m_previousMouseCoords.setCoords(-500, 0, 0, -500);

    // Update current layer to undefined
    m_currentLayer = -1;
    m_isDeleting = false;
    m_isMovingObject = false;

    // Update the undo redo
    m_controlUndoRedo.addState(m_map->mapProperties()->id(), m_changes);
}

// -------------------------------------------------------

void ControlMapEditor::onKeyPressed(int k, double speed) {
    if (!m_isDrawingWall && !m_isDeletingWall && !m_isMovingObject) {
        cursor()->onKeyPressed(k, m_camera->horizontalAngle(),
                               m_map->mapProperties()->length(),
                               m_map->mapProperties()->width(), speed);
    }
}

// -------------------------------------------------------

void ControlMapEditor::onKeyReleased(int) {

}
