#pragma once


namespace dal {

    class WindowGLFW {

    private:
        void* m_window = nullptr;

    public:
        WindowGLFW();
        ~WindowGLFW();

        void do_frame();
        bool should_close() const;

    };

}
