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
        if len(points) < 4:
            points.append((event.xdata, event.ydata))
        else:
            points[lastadded % 4] = (event.xdata, event.ydata)
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

import numpy as np

def dot2d(p0, p1):
    """Producto punto en 2D"""
    return p0[0] * p1[0] + p0[1] * p1[1]

def cross2d(p0, p1):
    """Producto cruzado en 2D"""
    return p0[0] * p1[1] - p0[1] * p1[0]

def calculateIntersection(p1, p2, p3, p4):
    # Convertir los puntos a arrays 1D de tipo float
    points = [np.array(p, dtype=np.float64).flatten() for p in [p1, p2, p3, p4]]
    
    for i in range(4):  # Punto base
        base = points[i]
        print(f"\n=== Punto base: {base} ===")
        
        for j in range(4):  # Otro punto para formar el vector r
            if i == j:
                continue  # No usar el mismo punto como referencia
            
            a, b = base, points[j]  # Dos puntos para formar el vector de referencia
            r = b - a  # Vector de referencia

            # Tomar los otros dos puntos
            remaining_points = [points[k] for k in range(4) if k != i and k != j]
            c, d = remaining_points

            cross_c = cross2d(r, c - a)  # Producto cruzado para el tercer punto
            cross_d = cross2d(r, d - a)  # Producto cruzado para el cuarto punto

            # Si los signos del producto cruzado son distintos, están en lados opuestos
            if cross_c * cross_d < 0:
                print(f"Con base en {a} y {b}: {c} y {d} están en lados opuestos ✅")
            else:
                print(f"Con base en {a} y {b}: {c} y {d} NO están en lados opuestos ❌")

def refresh():
    ax.clear()
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    ax.plot([p[0] for p in points], [p[1] for p in points], 'ro')

    if len(points) == 4:
        drawLine(points[0], points[1])
        drawLine(points[2], points[3])
        calculateIntersection(points[0], points[1], points[2], points[3])
        
        
    fig.canvas.draw()
    
def main():
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    # Connect the click event to the onclick function
    cid = fig.canvas.mpl_connect('button_press_event', onclick)
    # Show the plot 
    plt.show()

main()