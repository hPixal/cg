import numpy as np
import matplotlib.pyplot as plt

# Constants
points = []
lastadded = 0
fig, ax = plt.subplots()

def onclick(event):
    global lastadded
    global points
    if event.inaxes:
        if len(points) < 3:
            points.append((event.xdata, event.ydata))
        else:
            points[lastadded % 3] = (event.xdata, event.ydata)
        lastadded += 1
        refresh()
        
def drawLine(p1, p2):
    x1, y1 = p1
    x2, y2 = p2
    m = (y2 - y1) / (x2 - x1)
    b = y1 - m * x1
    x = np.linspace(0, 10, 100)
    y = m * x + b
    ax.plot(x, y, 'b-')
    
def drawSegment(p1, p2):
    x1, y1 = np.array(p1)
    x2, y2 = np.array(p2)
    m = (y2 - y1) / (x2 - x1)
    b = y1 - m * x1
    x = np.linspace( x1, x2 , 100)
    y = m * x + b
    ax.plot(x, y, 'b-')
    

def getDistance(p0,p1,p):
    p0 = np.array(p0)
    p1 = np.array(p1)
    p = np.array(p)
    
    # area = V0 X V1 || area = b * d
    # V0 x V1 = b * d
    # d = V0 x V1 / b
    # d = (p1-p0) x [ (p-p0)/||p-p0|| ]
    
    v0 = p1-p0
    v1 = p-p0
    
    b = np.linalg.norm(v0) # b = ||V0||
    d = np.cross(v0, v1*1/b) # a = V0 x V1
    print(d)
    
    
def getNormalPoint(p0,p1,p):
    p0 = np.array(p0)
    p1 = np.array(p1)
    p = np.array(p)
    
    v0 = p1-p0
    v1 = p-p0  
    
    # we need the projection of v1 into the direction of v0
    # to find the normal point we can do p0+|dot(v0,v1)|*(v0/|v0|)
    # |dot(v0,v1)| = |v0||v1|cos(theta)
    # cos(theta) = |dot(v0,v1)|/(|v0||v1|)
    
    v = v0*1/np.linalg.norm(v0)
    alpha = np.dot(v,v1)
    
    return (p0 + alpha*v);
    

def refresh():
    ax.clear()
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    ax.plot([p[0] for p in points], [p[1] for p in points], 'ro')

    if len(points) == 3:
        drawLine(points[0], points[1])
        getDistance(points[0],points[1],points[2])
        p_t = getNormalPoint(points[0],points[1],points[2])
        
        ax.plot(p_t[0],p_t[1],'go')
        drawSegment(p_t, points[2])
        
    fig.canvas.draw()
    
def main():
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    # Connect the click event to the onclick function
    cid = fig.canvas.mpl_connect('button_press_event', onclick)
    # Show the plot 
    plt.show()

main()