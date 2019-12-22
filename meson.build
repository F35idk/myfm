project ('com.github.f35idk.myfm', 'c',
        default_options: ['c_std=c11'],
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
            'myfm-file-menu.c',
            'myfm-directory-menu.c',
            'myfm-utils.c',
            'myfm-file-operations.c',
            dependencies : [gtk_dep, 
                            gio_dep,
                            glib_dep],
            install: true,
)
