constexpr ivec2 screen_size = ivec2(480, 270);
constexpr std::string_view window_name = "Iota";

Interface::Window window(std::string(window_name), screen_size * 2, Interface::windowed, adjust_(Interface::WindowSettings{}, min_size = screen_size));
Graphics::DummyVertexArray dummy_vao = nullptr;

const Graphics::ShaderConfig shader_config = Graphics::ShaderConfig::Core();
Interface::ImGuiController gui_controller(Poly::derived<Interface::ImGuiController::GraphicsBackend_Modern>, adjust_(Interface::ImGuiController::Config{}, shader_header = shader_config.common_header));

namespace Fonts
{
    namespace Files
    {
        Graphics::FontFile main("assets/Monocat_6x12.ttf", 12);
    }

    Graphics::Font main;
}

Graphics::TextureAtlas texture_atlas = []{
    Graphics::TextureAtlas ret(ivec2(2048), "assets/_images", "assets/atlas.png", "assets/atlas.refl");
    auto font_region = ret.Get("font_storage.png");

    Unicode::CharSet glyph_ranges;
    glyph_ranges.Add(Unicode::Ranges::Basic_Latin);

    Graphics::MakeFontAtlas(ret.GetImage(), font_region.pos, font_region.size, {
        {Fonts::main, Fonts::Files::main, glyph_ranges, Graphics::FontFile::monochrome_with_hinting},
    });
    return ret;
}();
Graphics::Texture texture_main = Graphics::Texture(nullptr).Wrap(Graphics::clamp).Interpolation(Graphics::nearest).SetData(texture_atlas.GetImage());

AdaptiveViewport adaptive_viewport(shader_config, screen_size);
Render r = adjust_(Render(0x2000, shader_config), SetTexture(texture_main), SetMatrix(adaptive_viewport.GetDetails().MatrixCentered()));

Input::Mouse mouse;

struct ProgramState : Program::DefaultBasicState
{
    GameUtils::State::StateManager state_manager;
    GameUtils::FpsCounter fps_counter;

    void Resize()
    {
        adaptive_viewport.Update();
        mouse.SetMatrix(adaptive_viewport.GetDetails().MouseMatrixCentered());
    }

    Metronome metronome = Metronome(60);

    Metronome *GetTickMetronome() override
    {
        return &metronome;
    }

    int GetFpsCap() override
    {
        return 60 * NeedFpsCap();
    }

    void EndFrame() override
    {
        fps_counter.Update();
        window.SetTitle(STR((window_name), " TPS:", (fps_counter.Tps()), " FPS:", (fps_counter.Fps())));
    }

    void Tick() override
    {
        // window.ProcessEvents();
        window.ProcessEvents({gui_controller.EventHook()});

        if (window.Resized())
        {
            Resize();
            Graphics::Viewport(window.Size());
        }
        if (window.ExitRequested())
            Program::Exit();

        gui_controller.PreTick();
        state_manager.Tick();
    }

    void Render() override
    {
        gui_controller.PreRender();
        adaptive_viewport.BeginFrame();
        state_manager.Render();
        adaptive_viewport.FinishFrame();
        Graphics::CheckErrors();
        gui_controller.PostRender();

        window.SwapBuffers();
    }


    void Init()
    {
        ImGui::StyleColorsDark();

        // Load various small fonts
        auto monochrome_font_flags = ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting;

        gui_controller.LoadFont("assets/Monocat_6x12.ttf", 12.0f, adjust(ImFontConfig{}, FontBuilderFlags = monochrome_font_flags));
        gui_controller.LoadDefaultFont();
        gui_controller.RenderFontsWithFreetype();

        Graphics::Blending::Enable();
        Graphics::Blending::FuncNormalPre();

        state_manager.NextState().Set("Initial");
    }
};

namespace States
{
    STRUCT( Initial EXTENDS GameUtils::State::BasicState )
    {
        UNNAMED_MEMBERS()

        float angle = 0;

        void Tick(const GameUtils::State::NextStateSelector &next_state) override
        {
            (void)next_state;

            angle += 0.01;
            ImGui::ShowDemoWindow();
        }

        void Render() const override
        {
            Graphics::SetClearColor(fvec3(0));
            Graphics::Clear();

            r.BindShader();

            r.iquad(mouse.pos(), ivec2(32)).center().rotate(angle).color(mouse.left.down() ? fvec3(1,0.5,0) : fvec3(0,0.5,1));

            r.Finish();
        }
    };
}

int _main_(int, char **)
{
    ProgramState program_state;
    program_state.Init();
    program_state.Resize();
    program_state.RunMainLoop();
    return 0;
}
