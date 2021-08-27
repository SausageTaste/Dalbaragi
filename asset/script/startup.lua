scene = require('scene')
logger = require('logger')
sysinfo = require('sysinfo')


function on_engine_init()
    do
        local light = scene.get_dlight_handle()
        light:set_direction_to_light(1, 10, 1);
        light:get_color():set_xyz(2, 2, 2)
    end

    do
        local light = scene.create_slight()
        light:get_pos():set_xyz(6, 2, 0)
        light:get_color():set_xyz(3, 3, 3)
        light:set_direction_to_light(0, 1, -1)
        light:set_fade_start_degree(0)
        light:set_fade_end_degree(35)
    end

    do
        local light = scene.create_slight()
        light:get_pos():set_xyz(6, 2, 0)
        light:get_color():set_xyz(5, 5, 5)
        light:set_direction_to_light(0, 0, 1)
        light:set_fade_start_degree(0)
        light:set_fade_end_degree(35)
    end

    do
        local light = scene.create_plight()
        light:get_color():set_xyz(0.5, 0.5, 0.5)
    end
end


function on_renderer_init()
    do
        local e = scene.create_actor_skinned('honoka_0', 'sungmin/honoka_basic_3.dmd')
        local t = e:get_transform()
        t:get_pos():set_x(-2)
        t:rotate_degree(90, 0, 1, 0)
        t:set_scale(0.3)
        e:notify_transform_change();
    end

    do
        local e = scene.create_actor_skinned('honoka_1', 'sungmin/honoka_basic_3.dmd')
        local t = e:get_transform()
        t:set_scale(0.3)
        e:notify_transform_change();
    end

    do
        local e = scene.create_actor_skinned('', '_asset/model/Character Running.dmd')
        local t = e:get_transform()
        t:get_pos():set_x(2)
        t:rotate_degree(90, 0, 1, 0);
        e:notify_transform_change();
        e:set_anim_index(0)
    end

    do
        local e = scene.create_actor_static('', '_asset/model/sponza.dmd')
        local t = e:get_transform()
        t:set_scale(0.01)
        t:rotate_degree(90, 1, 0, 0)
        e:notify_transform_change()
    end

    do
        local e = scene.create_actor_static('', '_asset/model/simple_box.dmd')
        local t = e:get_transform()
        t:get_pos():set_z(-1)
        e:notify_transform_change()
    end
end


function before_rendering()
    local t = sysinfo.time()

    do
        local light = scene.get_slight_at(0)
        light:set_direction_to_light(math.sin(t * 0.7), 1, math.cos(t * 0.7));
    end

    do
        local light = scene.get_slight_at(1)
        light:get_pos():set_xyz(math.cos(t*0.3) * 3, 1, 4.5);
    end

    do
        local light = scene.get_plight_at(0)
        light:get_pos():set_xyz(math.sin(t) * 3 - 10, 1, math.cos(t) * 2);
    end
end
