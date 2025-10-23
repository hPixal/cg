#include <algorithm>
#include <stdexcept>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Model.hpp"
#include "Window.hpp"
#include "Callbacks.hpp"
#include "Debug.hpp"
#include "Shaders.hpp"

#define VERSION 20250901

Window main_window; // ventana principal: muestra el modelo en 3d sobre el que se pinta
Window aux_window; // ventana auxiliar que muestra la textura

void drawMain(); // dibuja el modelo "normalmente" para la ventana principal
void drawBack(); // dibuja el modelo con un shader alternativo para convertir coords de la ventana a coords de textura
void drawAux(); // dibuja la textura en la ventana auxiliar
void drawImGui(Window &window); // settings sub-window

float radius = 5; // radio del "pincel" con el que pintamos en la textura
glm::vec4 color = { 0.f, 0.f, 0.f, 1.f }; // color actual con el que se pinta en la textura

Texture texture; // textura (compartida por ambas ventanas)
Image image; // imagen (para la textura, Image está en RAM, Texture la envía a GPU)

Model model_chookity; // el objeto a pintar, para renderizar en la ventan principal
Model model_aux; // un quad para cubrir la ventana auxiliar y mostrar la textura

Shader shader_main; // shader para el objeto principal (drawMain)
Shader shader_aux; // shader para la ventana auxiliar (drawTexture)

// callbacks del mouse y auxiliares para los callbacks
enum class MouseAction { None, ManipulateView, Draw };
MouseAction mouse_action = MouseAction::None; // qué hacer en el callback del motion si el botón del mouse está apretado
void mainMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void mainMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void auxMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void auxMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);


// Custom vars and variables
glm::vec2 getTexturePosition(GLFWwindow* window,glm::vec2 p);
void makeBrushSegment(glm::vec2 p0_t, glm::vec2 p1_t, bool debug = true);
glm::vec2 p0;
glm::vec2 p1;
void swapPoints();
void dda(glm::vec2 p_0,glm::vec2 p_1,glm::vec3 col);
bool start = false;
void endDrawing();
void blendPixel(int x, int y);
void scanlineFill(int y_min, int y_max, int x_min, int x_max);

struct TextureWindowConverter
{
    std::string method; // "CLAMP","REPEAT"

    TextureWindowConverter(std::string method = "CLAMP")
    {
        this->method = method;
    }

    // Window -> Texture normalized (s,t) where (0,0) is top-left and t increases downward
    glm::vec2 windowToTextureNormalized(glm::vec2 winPos, glm::vec2 windowSize) const
    {
        glm::vec2 uv = winPos / windowSize; // uv.y increases downward (same as window coords)

        if (method == "CLAMP")
        {
            uv.x = glm::clamp(uv.x, 0.0f, 1.0f);
            uv.y = 1 - glm::clamp(uv.y, 0.0f, 1.0f);
        }
        else if (method == "REPEAT")
        {
            uv.x = uv.x - std::floor(uv.x);
            uv.y = uv.y - std::floor(uv.y);
        }
        return uv;
    }

    // normalized (s,t) -> texture pixels (x,y) (origin top-left)
    glm::vec2 textureNormalizedToPixels(glm::vec2 texUV, glm::vec2 texSize) const
    {
        glm::vec2 px = texUV * texSize;
        // Clamp/repeat at pixel level if desired (already handled above)
        if (method == "CLAMP")
        {
            px.x = glm::clamp(px.x, 0.0f, texSize.x - 1.0f);
            px.y = glm::clamp(px.y, 0.0f, texSize.y - 1.0f);
        } else if (method == "REPEAT") {
            px.x = std::fmod(px.x, texSize.x);
            if (px.x < 0) px.x += texSize.x;
            px.y = std::fmod(px.y, texSize.y);
            if (px.y < 0) px.y += texSize.y;
        }
        return px;
    }

    // convenience: window pos -> texture pixels (both with origin top-left)
    glm::vec2 windowToTexturePixels(glm::vec2 winPos, glm::vec2 windowSize, glm::vec2 texSize) const
    {
        auto uv = windowToTextureNormalized(winPos, windowSize);
        return textureNormalizedToPixels(uv, texSize);
    }

    // (Si necesitás la inversa para debug)
    glm::vec2 texturePixelsToWindow(glm::vec2 texPx, glm::vec2 windowSize, glm::vec2 texSize) const
    {
        glm::vec2 uv = texPx / texSize;
        return uv * windowSize;
    }
};




int main() {
	
	// main window (3D view)
	main_window = Window(800, 600, "Main View", true);
	glfwSetCursorPosCallback(main_window, mainMouseMoveCallback);
	glfwSetMouseButtonCallback(main_window, mainMouseButtonCallback);
	main_window.getCamera().model_angle = 2.5;
	
	glClearColor(1.f,1.f,1.f,1.f);
	shader_main = Shader("shaders/main");
	
	image = Image("models/chookity.png",true);
	texture = Texture(image);
	
	model_chookity = Model::loadSingle("models/chookity", Model::fNoTextures);
	
	// aux window (texture image)
	aux_window = Window(512,512, "Texture", true, main_window);
	glfwSetCursorPosCallback(aux_window, auxMouseMoveCallback);
	glfwSetMouseButtonCallback(aux_window, auxMouseButtonCallback);
	
	model_aux = Model::loadSingle("models/texquad", Model::fNoTextures);
	shader_aux = Shader("shaders/quad");
	
	// main loop
	do {
		glfwPollEvents();
		
		glfwMakeContextCurrent(main_window);
		drawMain();
		drawImGui(main_window);
		glFinish();
		glfwSwapBuffers(main_window);
		
		glfwMakeContextCurrent(aux_window);
		drawAux();
		drawImGui(aux_window);
		glFinish();
		glfwSwapBuffers(aux_window);
		
	} while( (not glfwWindowShouldClose(main_window)) and (not glfwWindowShouldClose(aux_window)) );
}


// ===== pasos del renderizado =====

void drawMain() {
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	texture.bind();
	shader_main.use();
	setMatrixes(main_window, shader_main);
	shader_main.setLight(glm::vec4{-1.f,1.f,4.f,1.f}, glm::vec3{1.f,1.f,1.f}, 0.35f);
	shader_main.setMaterial(model_chookity.material);
	shader_main.setBuffers(model_chookity.buffers);
	model_chookity.buffers.draw();
}

void drawAux() {
	glDisable(GL_DEPTH_TEST);
	texture.bind();
	shader_aux.use();
	shader_aux.setMatrixes(glm::mat4{1.f}, glm::mat4{1.f}, glm::mat4{1.f});
	shader_aux.setBuffers(model_aux.buffers);
	model_aux.buffers.draw();
}

void drawBack() {
	glfwMakeContextCurrent(main_window);
	glDisable(GL_MULTISAMPLE);

	/// @ToDo: Parte 2: renderizar el modelo en 3d con un nuevo shader de forma 
	///                 que queden las coordenadas de textura de cada fragmento
	///                 en el back-buffer de color
	
	glEnable(GL_MULTISAMPLE);
	glFlush();
	glFinish();
}

void drawImGui(Window &window) {
	if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) return;
	// settings sub-window
	window.ImGuiDialog("Settings",[&](){
		ImGui::SliderFloat("Radius",&radius,1,50);
		ImGui::ColorEdit4("Color",&(color[0]),0);
		
		static std::vector<std::pair<const char *, ImVec4>> pallete = { // colores predefindos
			{"white" , {1.f,1.f,1.f,1.f}},
			{"pink"  , {0.749f,0.49f,0.498f,1.f}},
			{"yellow", {0.965f,0.729f,0.106f,1.f}},
			{"black" , {0.f,0.f,0.f,1.f}} };
		
		ImGui::Text("Pallete:");
		for (auto &p : pallete) {
			ImGui::SameLine();
			if (ImGui::ColorButton(p.first, p.second))
				color[0] = p.second.x, color[1] = p.second.y, color[2] = p.second.z;
		}
		
		if (ImGui::Button("Reload Image")) {
			image = Image("models/chookity.png",true);
			texture.update(image);
		}
	});
}










































// ===== callbacks de la ventana auxiliar (textura) =====

void auxMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (action==GLFW_PRESS) {
        mouse_action = MouseAction::Draw;

        // get cursor position (window coords, origin top-left)
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        p0.x = (float)xpos; p0.y = (float)ypos; // Save current position in p0 (window space).

    } else {
        // END DRAWING CASE
        if(mouse_action == MouseAction::Draw)
        { endDrawing(); }
        mouse_action = MouseAction::None;
    }
}

void auxMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    if (mouse_action!=MouseAction::Draw) return;

    // Update window-space p1
    p1.x = (float)xpos; p1.y = (float)ypos;

    // Convert both p0 and p1 (window coords) to texture pixels, then draw the brush segment in texture space
    auto p0_t = getTexturePosition(window, p0); // texture pixels
    auto p1_t = getTexturePosition(window, p1); // texture pixels

    // Draw in texture-pixel space
    makeBrushSegment(p0_t, p1_t, true);

    // push texture update to GPU
    texture.update(image);

    // advance p0 in window-space for next segment (keeps sampling consistent in case cursor moves)
    p0 = p1;
}


// ===== callbacks de la ventana principal (vista 3D) =====

void mainMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
	if (action==GLFW_PRESS) {
		if (mods!=0 or button==GLFW_MOUSE_BUTTON_RIGHT) {
			mouse_action = MouseAction::ManipulateView;
			common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
			return;
		}
		
		mouse_action = MouseAction::Draw;
		
		/// @ToDo: Parte 2: pintar un punto de radio "radius" en la imagen
		///                 "image" que se usa como textura
		
	} else {
		if (mouse_action==MouseAction::ManipulateView)
			common_callbacks::mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, action, mods);
		mouse_action = MouseAction::None;
	}
}

void mainMouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
	if (mouse_action!=MouseAction::Draw) {
		if (mouse_action==MouseAction::ManipulateView);
			common_callbacks::mouseMoveCallback(window,xpos,ypos);
		return; 
	}
	
	/// @ToDo: Parte 2: pintar un segmento de ancho "2*radius" en la imagen
	///                 "image" que se usa como textura
	
}

void makeBrushSegment(glm::vec2 p0_t, glm::vec2 p1_t, bool debug)
{
    // Trabajamos en espacio de textura (pixeles)
    float dx = p1_t.x - p0_t.x;
    float dy = p1_t.y - p0_t.y;
    if (dx == 0 && dy == 0) return;

    glm::vec2 a = p0_t;
    glm::vec2 b = p1_t;

    if (fabs(dx) > fabs(dy)) {
        if (a.x > b.x) std::swap(a, b);
    } else {
        if (a.y > b.y) std::swap(a, b);
    }

    // Normal al segmento
    auto nx = -(b.y - a.y);
    auto ny =  (b.x - a.x);
    auto len = std::sqrt(nx*nx + ny*ny);
    if (len == 0.0f) return;
    nx /= len;
    ny /= len;

    auto p0_u = a + round(radius * glm::vec2(nx, ny));
    auto p0_d = a - round(radius * glm::vec2(nx, ny));
    auto p1_u = b + round(radius * glm::vec2(nx, ny));
    auto p1_d = b - round(radius * glm::vec2(nx, ny));

    if (debug) {
        std::cout << "[makeBrushSegment] a=(" << a.x << "," << a.y << ") b=(" << b.x << "," << b.y << ")\n";
        std::cout << "normal=(" << nx << "," << ny << ") radius=" << radius << "\n";
    }

    // === Pintar borde en verde primero ===

    dda(p0_u, p1_u,glm::vec3(0,255,0) );
    dda(p0_d, p1_d,glm::vec3(0,255,0) );
    dda(p0_u, p0_d,glm::vec3(0,255,0) );
    dda(p1_u, p1_d,glm::vec3(0,255,0) );
	texture.update(image);

    // === Bounding box del trapecio ===
    int x_min = std::min({(int)p0_u.x, (int)p0_d.x, (int)p1_u.x, (int)p1_d.x});
    int x_max = std::max({(int)p0_u.x, (int)p0_d.x, (int)p1_u.x, (int)p1_d.x});
    int y_min = std::min({(int)p0_u.y, (int)p0_d.y, (int)p1_u.y, (int)p1_d.y});
    int y_max = std::max({(int)p0_u.y, (int)p0_d.y, (int)p1_u.y, (int)p1_d.y});

    // === Rellenar con scanline ===s
    scanlineFill(y_min, y_max, x_min, x_max);
	texture.update(image);
	
}

void scanlineFill(int y_min, int y_max, int x_min, int x_max) {
    for (int y = y_min; y <= y_max; y++) {
        bool inside = false;
        for (int x = x_min; x <= x_max; x++) {
            glm::vec3 c = image.GetRGB(y, x);
            if (c == glm::vec3(0,255,0)) {
                inside = !inside;
                continue;
            }
            if (inside) {
                blendPixel(x, y);
            }
        }
    }
}

// Algoritmo DDA para trazar una línea en la textura
void dda(glm::vec2 p_0,glm::vec2 p_1,glm::vec3 col)
{
	std::cout << "DDA COL: " << col.x << ", " << col.y << ", " << col.z << std::endl;
    float dx = p_1.x - p_0.x;
    float dy = p_1.y - p_0.y;

    if (dx == 0 && dy == 0) return;

    if (fabs(dx) > fabs(dy)) {
        // caso x-dominante
        if (p_0.x > p_1.x) std::swap(p_0, p_1);

        float slope = dy / dx;
        float y = p_0.y;
        for (int x = (int)p_0.x; x <= (int)p_1.x; x++) {
			image.SetRGB((int)round(y), x,glm::vec3(0,255,0));
			
            y += slope;
        }
    } else {
        // caso y-dominante
        if (p_0.y > p_1.y) std::swap(p_0, p_1);

        float slope = dx / dy;
        float x = p_0.x;
        for (int y = (int)p_0.y; y <= (int)p_1.y; y++) {
			image.SetRGB(y, (int)round(x),glm::vec3(0,255,0));
            x += slope;
        }
    }
}

void blendPixel(int x, int y) {
        glm::vec3 current_color = image.GetRGB(y, x); 
        float alpha = color.w;

        glm::vec3 src = glm::vec3(color.x, color.y, color.z);
        glm::vec3 dst = current_color;

        glm::vec3 out = alpha * src + (1.0f - alpha) * dst;
        image.SetRGB(y,x, out);
    };

void swapPoints(){
	glm::vec2 aux = p0;
	p0 = p1;
	p1 = aux;
}

glm::vec2 getTexturePosition(GLFWwindow* window,glm::vec2 p)
{
    // Use stack object (no new/delete)
    TextureWindowConverter converter("CLAMP");

    // Get window size
    int  win_width, win_height;
    glfwGetWindowSize(window,&win_width, &win_height);
    auto win_size = glm::vec2((float)win_width,(float)win_height);

    // Get texture size
    auto tex_size = glm::vec2( (float)image.GetWidth(), (float)image.GetHeight());

    // Translate window (x,y) -> texture pixels (x,y), origin top-left (no Y flip)
    auto p_t = converter.windowToTexturePixels(p, win_size, tex_size);

    if (false) { // poner true si querés debug por cada conversión
        std::cout << "[getTexturePosition] win=(" << p.x << "," << p.y << ") win_size=(" << win_size.x << "," << win_size.y << ")\n";
        std::cout << " -> tex_px=(" << p_t.x << "," << p_t.y << ") tex_size=(" << tex_size.x << "," << tex_size.y << ")\n";
    }

    return p_t;
}


void makeCircle(glm::vec2 center, float radius) {
    int x0 = (int)center.x;
    int y0 = (int)center.y;

    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                int px = x0 + x;
                int py = y0 + y;
                blendPixel(px, py);
            }
        }
    }
}

void endDrawing()
{

}
