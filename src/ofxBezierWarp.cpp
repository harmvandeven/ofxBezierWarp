/*
 * ofxBezierWarp.cpp
 *
 * Copyright 2013 (c) Matthew Gingold http://gingold.com.au
 * Adapted from: http://forum.openframeworks.cc/index.php/topic,4002.0.html
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * If you're using this software for something cool consider sending 
 * me an email to let me know about your project: m@gingold.com.au
 *
 */

#include "ofxBezierWarp.h"

GLfloat texpts [2][2][2] = {
    { {0, 0}, {1, 0} },	{ {0, 1}, {1, 1} }
};

//--------------------------------------------------------------
ofxBezierWarp::ofxBezierWarp(){
    numXPoints = 0;
    numYPoints = 0;
    warpX = 0;
    warpY = 0;
    warpWidth = 0;
    warpHeight = 0;
    bShowWarpGrid = false;
    bWarpPositionDiff = false;
    bDoWarp = true;
}

//--------------------------------------------------------------
ofxBezierWarp::~ofxBezierWarp(){
    //fbo.destroy();
    dragOffsetPoints.clear();
    selectedPoints.clear();
    cntrlPoints.clear();
}

//--------------------------------------------------------------
void ofxBezierWarp::allocate(int _w, int _h, int pixelFormat){
    allocate(_w, _h, 2, 2, 100.0f, pixelFormat);
}

//--------------------------------------------------------------
void ofxBezierWarp::allocate(int _w, int _h, int _numXPoints, int _numYPoints, float pixelsPerGridDivision, int pixelFormat){

    //disable arb textures (so we use texture 2d instead)

    if(_w == 0 || _h == 0 || _numXPoints == 0 || _numYPoints == 0){
        ofLogError() << "Cannot accept 0 as value for w, h numXPoints or numYPoints";
        return;
    }

    if(_w != fbo.getWidth() || _h != fbo.getHeight()){

        fbo.allocate(_w, _h, pixelFormat);
        ofLogVerbose() << "Allocating bezier fbo texture as: " << fbo.getWidth() << " x " << fbo.getHeight();
    }

    setWarpGrid(_numXPoints, _numYPoints);

    //set up texture map for bezier surface
    glMap2f(GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2, &texpts[0][0][0]);
    glEnable(GL_MAP2_TEXTURE_COORD_2);
    glEnable(GL_MAP2_VERTEX_3);
    glEnable(GL_AUTO_NORMAL);

    setWarpGridResolution(pixelsPerGridDivision);
    
    //glShadeModel(GL_FLAT);

}

//--------------------------------------------------------------
void ofxBezierWarp::loadWarpGrid()
{
    grid.open( file );
    for ( int i=0; i<grid.size(); i++ )
    {
        cntrlPoints.at(i) = grid[i].asFloat();
    }
}

//--------------------------------------------------------------
void ofxBezierWarp::saveWarpGrid()
{
    if(!bShowWarpGrid) return;
    
    grid.clear();
    for ( int i=0; i<cntrlPoints.size(); i++ )
    {
        grid.append( cntrlPoints.at(i) );
    }
    grid.save( file );
}

//--------------------------------------------------------------
void ofxBezierWarp::setWarpGridFile(string _file)
{
    file = _file;
}

//--------------------------------------------------------------
void ofxBezierWarp::selectAllWarpGrid()
{
    if(!bShowWarpGrid) return;
    
    for(int i = 0; i < numYPoints; i++)
    {
        for(int j = 0; j < numXPoints; j++)
        {
            selectedPoints[i*numXPoints+j] = true;
        }
    }
}

//--------------------------------------------------------------
void ofxBezierWarp::begin(){
    fbo.begin();
    ofPushMatrix();
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

//--------------------------------------------------------------
void ofxBezierWarp::end(){
    ofPopMatrix();
    fbo.end();
}

//--------------------------------------------------------------
void ofxBezierWarp::draw(){
    draw(0, 0, fbo.getWidth(), fbo.getHeight());
}

//--------------------------------------------------------------
void ofxBezierWarp::draw(float x, float y){
    draw(x, y, fbo.getWidth(), fbo.getHeight());
}

//--------------------------------------------------------------
void ofxBezierWarp::draw(float x, float y, float w, float h){

    if(!fbo.isAllocated()) return;

    ofPushMatrix();

    if(bDoWarp){
        
        ofTranslate(x, y);
        ofScale(w/fbo.getWidth(), h/fbo.getHeight());
        
        ofTexture & fboTex = fbo.getTextureReference();
        
        // upload the bezier control points to the map surface
        // this can be done just once (or when control points change)
        // if there is only one bezier surface - but with multiple
        // it needs to be done every frame
        glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, numXPoints, 0, 1, numXPoints * 3, numYPoints, &(cntrlPoints[0]));
        
        fboTex.bind();
        
        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        glLoadIdentity();
        
        glScalef(fboTex.getWidth(), fboTex.getHeight(), 1.0f);
        glMatrixMode(GL_MODELVIEW);
        
//      glEnable(GL_MAP2_VERTEX_3);
//      glEnable(GL_AUTO_NORMAL);
        glEvalMesh2(GL_FILL, 0, gridDivX, 0, gridDivY);
//      glDisable(GL_MAP2_VERTEX_3);
//      glDisable(GL_AUTO_NORMAL);
        
        fboTex.unbind();
        
        glMatrixMode(GL_TEXTURE);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        
    }else{
        
        fbo.draw(x, y, w, h);
        
    }

    ofPopMatrix();

    if(bShowWarpGrid){
        if(bWarpPositionDiff){
            drawWarpGrid(warpX, warpY, warpWidth, warpHeight);
        }else{
            setWarpGridPosition(x, y, w, h);
        }
    }
}

//--------------------------------------------------------------
void ofxBezierWarp::moveSelectionWarpGrid(float x, float y)
{
    if(!bShowWarpGrid) return;
    
    for(int i = 0; i < numYPoints; i++)
    {
        for(int j = 0; j < numXPoints; j++)
        {
            if ( selectedPoints[i*numXPoints+j] )
            {
                cntrlPoints[(i*numXPoints+j)*3+0] += x;
                cntrlPoints[(i*numXPoints+j)*3+1] += y;
            }
        }
    }
}

//--------------------------------------------------------------
void ofxBezierWarp::drawWarpGrid(float x, float y, float w, float h){

    ofPushStyle();
    ofPushMatrix();

    ofSetColor(255, 255, 255);
    ofTranslate(x, y);
    ofScale(w/fbo.getWidth(), h/fbo.getHeight());

//    glEnable(GL_MAP2_VERTEX_3);
//    glEnable(GL_AUTO_NORMAL);
    glEvalMesh2(GL_LINE, 0, gridDivX, 0, gridDivY);
//    glDisable(GL_MAP2_VERTEX_3);
//    glDisable(GL_AUTO_NORMAL);
    
    for(int i = 0; i < numYPoints; i++){
        for(int j = 0; j < numXPoints; j++){
            ofFill();
            ofSetColor(255, 0, 0);
            if ( selectedPoints[(i*numXPoints+j)] ) ofSetColor(0, 255, 0);
            ofCircle(cntrlPoints[(i*numXPoints+j)*3+0], cntrlPoints[(i*numXPoints+j)*3+1], 5);
            ofNoFill();
        }
    }
    if ( bSelectBox )
    {
        ofSetLineWidth(2.0);
        ofSetColor(0,255,0);
        ofRect( dragInitX, dragInitY, dragX-dragInitX, dragY-dragInitY );
    }

    ofPopMatrix();
    ofPopStyle();
}

//--------------------------------------------------------------
void ofxBezierWarp::setWarpGrid(int _numXPoints, int _numYPoints, bool forceReset){

    if(_numXPoints != numXPoints || _numYPoints != numYPoints) forceReset = true;

    if(_numXPoints < 2 || _numYPoints < 2){
        ofLogError() << "Can't have less than 2 X or Y grid points";
        return;
    }

    if(forceReset){

        numXPoints = _numXPoints;
        numYPoints = _numYPoints;

        // calculate an even distribution of X and Y control points across fbo width and height
        cntrlPoints.resize(numXPoints * numYPoints * 3);
        dragOffsetPoints.resize(numXPoints * numYPoints * 3);
        selectedPoints.resize(numXPoints * numYPoints );
        for(int i = 0; i < numYPoints; i++){
            GLfloat x, y;
            y = (fbo.getHeight() / (numYPoints - 1)) * i;
            for(int j = 0; j < numXPoints; j++){
                x = (fbo.getWidth() / (numXPoints - 1)) * j;
                cntrlPoints[(i*numXPoints+j)*3+0] = x;
                cntrlPoints[(i*numXPoints+j)*3+1] = y;
                cntrlPoints[(i*numXPoints+j)*3+2] = 0;
                //cout << x << ", " << y << ", " << "0" << endl;
            }
        }
    }

    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, numXPoints, 0, 1, numXPoints * 3, numYPoints, &(cntrlPoints[0]));
}

//--------------------------------------------------------------
void ofxBezierWarp::setWarpGridPosition(float x, float y, float w, float h){
    warpX = x;
    warpY = y;
    warpWidth = w;
    warpHeight = h;
    bWarpPositionDiff = true;
}

//--------------------------------------------------------------
void ofxBezierWarp::setWarpGridResolution(float pixelsPerGridDivision){
    setWarpGridResolution(ceil(fbo.getWidth() / pixelsPerGridDivision), ceil(fbo.getHeight() / pixelsPerGridDivision));
}

//--------------------------------------------------------------
void ofxBezierWarp::setWarpGridResolution(int gridDivisionsX, int gridDivisionsY){
    // NB: at the moment this sets the resolution for all mapGrid
    // objects (since I'm not calling it every frame as it is expensive)
    // so if you try to set different resolutions
    // for different instances it won't work as expected

    gridDivX = gridDivisionsX;
    gridDivY = gridDivisionsY;
    glMapGrid2f(gridDivX, 0, 1, gridDivY, 0, 1);
}

//--------------------------------------------------------------
void ofxBezierWarp::resetWarpGrid(){
    setWarpGrid(numXPoints, numYPoints, true);
}

//--------------------------------------------------------------
void ofxBezierWarp::resetWarpGridPosition(){
    bWarpPositionDiff = false;
}

//--------------------------------------------------------------
float ofxBezierWarp::getWidth(){
    return fbo.getWidth();
}

//--------------------------------------------------------------
float ofxBezierWarp::getHeight(){
    return fbo.getHeight();
}

//--------------------------------------------------------------
int ofxBezierWarp::getNumXPoints(){
    return numXPoints;
}

//--------------------------------------------------------------
int ofxBezierWarp::getNumYPoints(){
    return numYPoints;
}

//--------------------------------------------------------------
int ofxBezierWarp::getGridDivisionsX(){
    return gridDivX;
}

//--------------------------------------------------------------
int ofxBezierWarp::getGridDivisionsY(){
    return gridDivY;
}

//--------------------------------------------------------------
void ofxBezierWarp::toggleShowWarpGrid(){
    setShowWarpGrid(!getShowWarpGrid());
}

//--------------------------------------------------------------
void ofxBezierWarp::setShowWarpGrid(bool b){
    bShowWarpGrid = b;
    if(bShowWarpGrid){
        ofRegisterMouseEvents(this);
    }else{
        ofUnregisterMouseEvents(this);
    }
}

//--------------------------------------------------------------
bool ofxBezierWarp::getShowWarpGrid(){
    return bShowWarpGrid;
}

//--------------------------------------------------------------
void ofxBezierWarp::setDoWarp(bool b){
    bDoWarp = b;
}

//--------------------------------------------------------------
bool ofxBezierWarp::getDoWarp(){
    return bDoWarp;
}

//--------------------------------------------------------------
void ofxBezierWarp::toggleDoWarp(){
    bDoWarp = !bDoWarp;
}

//--------------------------------------------------------------
ofFbo& ofxBezierWarp::getFBO(){
    return fbo;
}

//--------------------------------------------------------------
ofTexture& ofxBezierWarp::getTextureReference(){
    return fbo.getTextureReference();
}

//--------------------------------------------------------------
void ofxBezierWarp::setControlPoints(vector<GLfloat> _cntrlPoints){
    cntrlPoints.clear();
    cntrlPoints = _cntrlPoints;
    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, numXPoints, 0, 1, numXPoints * 3, numYPoints, &(cntrlPoints[0]));
}

//--------------------------------------------------------------
vector<GLfloat> ofxBezierWarp::getControlPoints(){
    return cntrlPoints;
}

//--------------------------------------------------------------
vector<GLfloat>& ofxBezierWarp::getControlPointsReference(){
    return cntrlPoints;
}

//--------------------------------------------------------------
void ofxBezierWarp::mouseMoved(ofMouseEventArgs & e){

}

//--------------------------------------------------------------
void ofxBezierWarp::mouseDragged(ofMouseEventArgs & e){

    if(!bShowWarpGrid) mouseReleased(e);

    float x = e.x;
    float y = e.y;
    if(bWarpPositionDiff)
    {
        x = (e.x - warpX) * fbo.getWidth()/warpWidth;
        y = (e.y - warpY) * fbo.getHeight()/warpHeight;
    }
    dragX   = x;
    dragY   = y;
    
    if( !bSelectBox )
    {
        // Dragging points
        float offsetX = 0;
        float offsetY = 0;
        
        for(int i = 0; i < numYPoints; i++)
        {
            for(int j = 0; j < numXPoints; j++)
            {
                if ( selectedPoints[i*numXPoints+j] )
                {
                    cntrlPoints[(i*numXPoints+j)*3+0] = x + dragOffsetPoints[(i*numXPoints+j)*3+0];
                    cntrlPoints[(i*numXPoints+j)*3+1] = y + dragOffsetPoints[(i*numXPoints+j)*3+1];
                }
            }
        }
    }
    else
    {
        // Drag a selectbox to select multiple cntrlPoints
        for(int i = 0; i < numYPoints; i++)
        {
            for(int j = 0; j < numXPoints; j++)
            {
                float xpos = cntrlPoints[(i*numXPoints+j)*3+0];
                float ypos = cntrlPoints[(i*numXPoints+j)*3+1];
                selectedPoints[i*numXPoints+j] = false;
                
                // Add points within the selectbox
                if ( (dragInitX > x && xpos >= x && xpos <= dragInitX) || (dragInitX < x && xpos >= dragInitX && xpos <= x) )
                {
                    if ( (dragInitY > y && ypos >= y && ypos <= dragInitY) || (dragInitY < y && ypos >= dragInitY && ypos <= y) )
                    {
                        selectedPoints[i*numXPoints+j] = true;
                    }
                }
                
                // If the mouse is at the edge of the screen also add points outside the screen
                if ( (dragInitY > y && ypos >= y && ypos <= dragInitY) || (dragInitY < y && ypos >= dragInitY && ypos <= y) )
                {
                    if ( (dragInitX > x && x < 5.0 && xpos < 5.0) || (dragInitX < x && x > ofGetWidth()-5.0 && xpos > ofGetWidth()-5.0) )
                    {
                        selectedPoints[i*numXPoints+j] = true;
                    }
                }
                if ( (dragInitX > x && xpos >= x && xpos <= dragInitX) || (dragInitX < x && xpos >= dragInitX && xpos <= x) )
                {
                    if ( (dragInitY > y && y < 5.0 && ypos < 5.0) || (dragInitY < y && y > ofGetHeight()-5.0 && ypos > ofGetHeight()-5.0) )
                    {
                        selectedPoints[i*numXPoints+j] = true;
                    }
                }
            }
        }
    }
    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, numXPoints, 0, 1, numXPoints * 3, numYPoints, &(cntrlPoints[0]));
    
}

//--------------------------------------------------------------
void ofxBezierWarp::mousePressed(ofMouseEventArgs & e){

    if(!bShowWarpGrid) mouseReleased(e);

    float x     = e.x;
    float y     = e.y;
    
    if (bWarpPositionDiff)
    {
        x = (e.x - warpX) * fbo.getWidth()/warpWidth;
        y = (e.y - warpY) * fbo.getHeight()/warpHeight;
    }
    
    dragX       = x;
    dragY       = y;
    dragInitX   = x;
    dragInitY   = y;

   

    float dist  = 10.0f;
    int   hit   = -1;
    for(int i = 0; i < numYPoints; i++)
    {
        for(int j = 0; j < numXPoints; j++)
        {
            if( x - cntrlPoints[(i*numXPoints+j)*3+0] >= -dist && x - cntrlPoints[(i*numXPoints+j)*3+0] <= dist &&
               y - cntrlPoints[(i*numXPoints+j)*3+1] >= -dist && y - cntrlPoints[(i*numXPoints+j)*3+1] <= dist)
            {
                hit = (i*numXPoints+j);
            }
        }
    }
    
    if (hit < 0)
    {
        bSelectBox  = true;
        for(int i = 0; i < numYPoints; i++)
        {
            for(int j = 0; j < numXPoints; j++)
            {
                selectedPoints[(i*numXPoints+j)] = false;
            }
        }
    }
    else
    {
        for(int i = 0; i < numYPoints; i++)
        {
            for(int j = 0; j < numXPoints; j++)
            {
                dragOffsetPoints[(i*numXPoints+j)*3+0] = cntrlPoints[(i*numXPoints+j)*3+0] - cntrlPoints[hit*3+0];
                dragOffsetPoints[(i*numXPoints+j)*3+1] = cntrlPoints[(i*numXPoints+j)*3+1] - cntrlPoints[hit*3+1];
            }
        }
        bSelectBox          = false;
        dragInitSelect      = hit;
        selectedPoints[hit] = true;
    }
    
}

//--------------------------------------------------------------
void ofxBezierWarp::mouseReleased(ofMouseEventArgs & e)
{
    bSelectBox = false;
}
