//
//  Node.cpp
//  IMDG
//
//  Created by CNPP on 22.5.2011.
//  Copyright 2011 Beat Raess. All rights reserved.
//
#include "Node.h"
#include "cinder/Text.h"
#include "cinder/Rand.h"
#include "cinder/CinderMath.h"


#pragma mark -
#pragma mark Object

/**
 * Creates a Node.
 */
Node::Node() {
    Node("nid",0,0);
}
Node::Node(string idn, double x, double y) {
    GLog();
    
    // node
    nid = idn;
    parent = NodeWeakPtr();
    label = " ";
    type = "Node";
    
    
    // fields
    perimeter = 390;
    dist = 420;
    damping = 0.5;
    strength = -1;
    ramp = 1.0;
    mvelocity = 10;
    speed = 45;
    nbchildren = 12;
    fcount = 0;
    
    // position
    pos.set(x,y);
    ppos.set(x,y);
    mpos.set(x,y);
    
    // state
    selected = false;
    active = false;
    grow = false;
    loading = false;
    visible = false;
    
    // radius / mass
    core = 9;
    radius = 9;
    mradius = 90;
    mass = calcmass();
    
    // velocity
    velocity.set(0,0);
    
    // color
    cbg = Color(1,1,1);
    ctxt = Color(0.85,0.85,0.85);
    ctxts = Color(1,1,1);
    
    // alpha
    acore = 0.6;
    ascore = 1.0;
    aglow = 0.3;
    asglow = 0.45;

    
    // font
    font = Font("Helvetica",12);
    textureLabel = gl::Texture(1,1);
    loff.set(0,5);

}

/**
 * Node movie.
 */
NodeMovie::NodeMovie(): Node::Node()  {    
}
NodeMovie::NodeMovie(string idn, double x, double y): Node::Node(idn, x, y) {
    
    // type
    this->updateType(nodeMovie);

}

/**
 * Node person.
 */
NodePerson::NodePerson(): Node::Node()  {    
}
NodePerson::NodePerson(string idn, double x, double y): Node::Node(idn, x, y) {
    
    // type
    this->updateType(nodePerson);
}



#pragma mark -
#pragma mark Cinder

/**
 * Applies the settings.
 */
void Node::setting(GraphSettings s) {
    
    
    // children
    nbchildren = 12;
    Default graphNodeChildren = s.getDefault("graph_node_children");
    if (graphNodeChildren.isSet()) {
        nbchildren = (int) graphNodeChildren.doubleVal();
    }
    
    
    // distance
    dist = 420;
    double length = 400;
    Default graphEdgeLength = s.getDefault("graph_edge_length");
    if (graphEdgeLength.isSet()) {
        length = graphEdgeLength.doubleVal();
        dist = length * 1.05;
    }
    
    
    
    // perimeter
    perimeter = 390;
    Default graphNodePerimeter = s.getDefault("graph_node_perimeter");
    if (graphNodePerimeter.isSet()) {
        perimeter = graphNodePerimeter.doubleVal();
    }
    perimeter = min(perimeter,length);
}


#pragma mark -
#pragma mark Sketch

/**
* Updates the node.
*/
void Node::update() {
    
    // count
    fcount++;
    
    
    // limit
    velocity.limit(mvelocity);
    
    // threshold
    float thresh = 0.1;
    if (abs(velocity.x) < thresh && abs(velocity.y) < thresh) {
        velocity.x = 0;
        velocity.y = 0;
    }

    // damping
    velocity *= (1 - damping);
    
    // add vel to moving position
    mpos += velocity;
    
    // update position
    Vec2d dm = mpos - pos;
    ppos = pos;
    pos += dm/speed;

    // grow
    if (grow) {
        
        // radius
        radius += 1;
        
        // mass
        mass = calcmass();
        
        // grown
        if (radius >= growradius) {
            this->grown();
        }
        
        
    }

    
}




/**
* Draws the node.
*/
void Node::draw() {
    
    
    // blend
    gl::enableAlphaBlending();
    

    // glow
    float ga = selected ? asglow : aglow;
    if (loading && ! grow) {
        ga *= (1.5+sin((fcount*1.5*M_PI)/180)); 
    }
    glColor4f(cbg.r,cbg.g,cbg.b,ga);
    gl::drawSolidCircle(pos, radius);
    
    // core
    glColor4f(cbg.r,cbg.g,cbg.b,(selected ? ascore : acore));
    gl::drawSolidCircle(pos, core);

    
    // unblend
    gl::enableAlphaBlending(true);
    
    // label
    selected ? gl::color(ctxts) : gl::color(ctxt);
	gl::draw(textureLabel, Vec2d(pos.x+loff.x, pos.y+radius+loff.y));

}




#pragma mark -
#pragma mark Business

/**
* Node repulsion.
*/
void Node::attract(NodePtr node) {
    

    // distance
    double d = pos.distance((*node).pos);
    double p = active ? perimeter : radius*1.25;
    if (d > 0 && d < p) {
        
        // force
        double s = pow(d / p, 1 / ramp);
        double m = selected ? mass*2 : mass;
        double force = s * 9 * strength * (1 / (s + 1) + ((s - 3) / 4)) / d;
        Vec2d df = (pos - (*node).pos) * (force/m);
        
        // velocity
        (*node).velocity += df;
    }

}


/**
 * Move.
 */
void Node::move(double dx, double dy) {
    mpos.x += dx;
    mpos.y += dy;
}
void Node::move(Vec2d d) {
    mpos += d;
}
void Node::moveTo(double x, double y) {
    mpos.x = x;
    mpos.y = y;
}
void Node::moveTo(Vec2d p) {
    mpos.set(p);
}

/**
 * Translate.
 */
void Node::translate(Vec2d d) {
    pos += d;
    mpos += d;
}


/**
* Adds a child.
*/
void Node::addChild(NodePtr child) {
    GLog();
    
    // push
    children.push_back(child);
}

/**
 * Node is grown.
 */
void Node::grown() {
    FLog();
    
    // state
    loading = false;
    grow = false;
    

    // mass
    mass = calcmass();
    
    // state
    active = true;
    
    // unfold
    this->unfold();
      
}

/**
 * Unfold.
 */
void Node::unfold() {
    
    // children
    int nb = nbchildren;
    Rand::randomize();
    for (NodeIt child = children.begin(); child != children.end(); ++child) {
        
        // parent
        if ((*child)->parent.lock()) {
            
            // unhide
            (*child)->show(false);
            
        }
        
        // adopt child
        else {
            
            // adopt child
            (*child)->parent = sref;
            
            // director
            if ((*child)->type == nodePersonDirector) {
                (*child)->show(true);
            }
            
            // actor / movie
            if (((*child)->type == nodePersonActor || (*child)->type == nodeMovie)) {
                
                // show
                if (nb > 0) {
                    (*child)->show(true);
                    nb--;
                }
                else if (! (*child)->isActive()) {
                   (*child)->hide(); 
                }
            }

            
        }
        
    }
}

/**
 * Load noad.
 */
void Node::load() {
    FLog();
    
    // state
    visible = true;
    loading = true;
    
    // radius
    radius = 30;
    core = 15;
    
    // color
    ctxt = Color(0.9,0.9,0.9);
    
    // font
    font = Font("Helvetica-Bold",15);
    loff.y = 6;
    this->renderLabel(label);

    
}
void Node::loaded() {
    FLog();

    // field
    growradius = min((int)children.size()*3,mradius);
    grow = true;
  
}


/**
 * Shows/Hides the node.
 */
void Node::show(bool animate) {
    GLog();
    
    // show it
    if (! visible) {
        
        // parent
        NodePtr pp = this->parent.lock();
        if (pp) {
            
            // radius & position
            float r = pp->radius * 0.5;
            Vec2d p = Vec2d(pp->pos.x+Rand::randFloat(-r,r),pp->pos.y+Rand::randFloat(-r,r));
            
            // animate
            if (animate) {
                this->pos.set(pp->pos);
                this->moveTo(p);
            }
            // set position
            else {
                this->pos.set(p);
                this->mpos.set(p);
            }
        }
        
    }
    
    // state
    visible = true;
    
}
void Node::hide() {
    GLog();
    
    // state
    visible = false;
    
}



/**
 * Touched.
 */
void Node::touched() {
    GLog();
    
    // state
    selected = true;
    
}
void Node::untouched() {
    GLog();
    
    // state
    selected = false;
    
}


/**
 * Tapped.
 */
void Node::tapped() {
    FLog();
    
    // state
    selected = false;
    
    // reposition
    if (! active && ! loading) {
        
        // parent
        NodePtr pp = this->parent.lock();
        if (pp) {
            
            // distance to parent
            Vec2d pdist =  pos - pp->pos;
            if (pdist.length() < dist) {
                
                // unity vector
                pdist.safeNormalize();
                
                // move
                this->moveTo(pp->pos+pdist*dist);
            }
        }
    
    }
    
}


/**
 * States.
 */
bool Node::isActive() {
    return active;
}
bool Node::isVisible() {
    return visible;
}
bool Node::isSelected() {
    return selected;
}
bool Node::isLoading() {
    return loading;
}



/**
 * Renders the label.
 */
void Node::renderLabel(string lbl) {
    GLog();
    
    // field
    label = (lbl == "") ? " " : lbl;
    
    // text
    TextLayout tlLabel;
    tlLabel.clear(ColorA(0, 0, 0, 0));
    tlLabel.setFont(font);
    tlLabel.setColor(ctxt);
    tlLabel.addCenteredLine(label);
    Surface8u rendered = tlLabel.render(true, true);
    textureLabel = gl::Texture(rendered);
    
    // offset
    loff.x = - textureLabel.getWidth() / 2.0;

}

/**
 * Updates the type.
 */
void Node::updateType(string t) {
    
    // movie
    if (t == nodeMovie) {
        
        // type
        type = t;
        
        // color 
        cbg = Color(138/255.0, 134/255.0, 96/255.0);
    }
    // person actor
    else if (t == nodePersonActor) {
        
        // type
        type = t;
        
        // color 
        cbg = Color(88/255.0, 124/255.0, 138/255.0);
    }
    // person director/crew
    else if (t == nodePersonCrew || t == nodePersonDirector || t == nodePerson) {
        
        // type
        type = t;
        
        // color 
        cbg = Color(131/255.0, 136/255.0, 138/255.0);
    }
}


/*
 * Calculates the mass.
 */
float Node::calcmass() {
    return radius * radius * 0.0001f + 0.01f;
}



