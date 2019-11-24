project ('com.github.f35idk.myfm', 'c',
        default_options: ['c_std=c11'],
)

compiler = meson.get_compiler ('c')
test_code = '''
            #if __has_attribute (__cleanup__)
            #   define RETURN_VALUE 0
            #endif
            int main (void) { return RETURN_VALUE; }
            '''
assert (compiler.compiles (test_code),
        'compiler does not support __attribute__ cleanup'
)

gtk_dep = dependency ('gtk+-3.0') # require version
gio_dep = dependency ('gio-2.0') # require version
glib_dep = dependency ('glib-2.0') # require version

executable (meson.project_name (),
            'myfm-main.c',
            'myfm-application.c',
            'myfm-window.c',
            'myfm-directory-view.c',
            'myfm-file.c',
            'myfm-multi-paned.c',
            'myfm-context-menu.c',
            dependencies : [gtk_dep, 
                            gio_dep,
                            glib_dep],
            install: true,
)
