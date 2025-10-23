# Python code to create a pixel-grid and a DDA rasterizer, then demonstrate usage.
import numpy as np
import matplotlib.pyplot as plt

class PixelGrid:
    """
    Simple 2D pixel grid for rasterization experiments.
    - width, height: integer pixel dimensions
    - dtype: numpy dtype for the buffer (default uint8)
    The buffer stores 0 for off, and positive integers for different "colors"/values.
    """
    def __init__(self, width: int, height: int, dtype=np.uint8):
        assert width > 0 and height > 0, "width and height must be positive integers"
        self.width = width
        self.height = height
        self.dtype = dtype
        self.buffer = np.zeros((height, width), dtype=dtype)  # row-major: buffer[y, x]
    
    def clear(self, value=0):
        """Clear the buffer to `value` (default 0)."""
        self.buffer.fill(value)
    
    def set_pixel(self, x: int, y: int, value=1):
        """Set a pixel at integer coordinates (x,y). Coordinates outside the grid are ignored."""
        if 0 <= x < self.width and 0 <= y < self.height:
            self.buffer[y, x] = value
    
    def get_pixel(self, x: int, y: int):
        """Return value at (x,y) or None if out of bounds."""
        if 0 <= x < self.width and 0 <= y < self.height:
            return int(self.buffer[y, x])
        return None
    
    def to_numpy(self):
        """Return the raw numpy buffer (copy)."""
        return self.buffer.copy()
    
    def show(self, title="PixelGrid", figsize=(6,6), cmap=None, interpolation='nearest', origin='lower'):
        """
        Display the pixel buffer using matplotlib.
        Note: Do not force colors — matplotlib will pick the default mapping unless you pass `cmap`.
        """
        plt.figure(figsize=figsize)
        plt.imshow(self.buffer, origin=origin, interpolation=interpolation, cmap=cmap)
        plt.title(title)
        plt.xlabel("x (pixels)")
        plt.ylabel("y (pixels)")
        plt.gca().set_aspect('equal')
        plt.grid(False)
        plt.show()
    
    def ascii_art(self, on_char='#', off_char='.', invert=False):
        """Return an ASCII representation (string) of the grid. Useful for small grids."""
        lines = []
        for y in range(self.height-1, -1, -1):  # print top row first
            row = []
            for x in range(self.width):
                v = self.buffer[y, x]
                on = v != 0
                if invert:
                    on = not on
                row.append(on_char if on else off_char)
            lines.append(''.join(row))
        return '\n'.join(lines)
    
    def draw_line_dda(self, x0, y0, x1, y1, value=1):
        dx = x1-x0
        dy = y1-y0
        
        if(abs(dx) > abs(dy)):
            if dx < 0:
                x0,x1 = x1,x0
                dx = -dx
            x = round(x0)
            y = round(y0)
            while(x <= x1):
                x+=1
                y+= dy/dx
                self.set_pixel(x,round(y),value)
        else:
            if dy < 0:
                y0,y1 = y1,y0
                dy = -dy
            x = round(x0)
            y = round(y0)
            while(y <= y1):
                y+=1
                x+= dx/dy
                self.set_pixel(round(x),y,value)

# --- Example usage / demo ---
if __name__ == "__main__":
    # Create a 64x48 pixel grid
    grid = PixelGrid(64, 48)
    
    # Draw several lines with different slopes to test DDA
    grid.clear()
    grid.draw_line_dda(2, 2, 60, 40, value=255)    # gentle positive slope
    grid.draw_line_dda(2, 40, 60, 2, value=200)    # negative slope
    grid.draw_line_dda(32, 0, 32, 47, value=180)   # vertical
    grid.draw_line_dda(0, 24, 63, 24, value=120)   # horizontal
    grid.draw_line_dda(10.5, 10.5, 20.2, 30.8, value=220)  # subpixel endpoints
    
    # Show ASCII preview (small size)
    print("ASCII preview (top row first):\n")
    print(grid.ascii_art(on_char='X', off_char='.', invert=False))
    
    # Show the grid visually
    grid.show(title="DDA Rasterization Demo (64x48)")
