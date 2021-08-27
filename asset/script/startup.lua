scene = require('scene')
logger = require('logger')


function on_engine_init()
    local light = scene.get_dlight_handle()
    light:set_direction_to_light(1, 10, 1);
    light:get_color():set_xyz(2, 2, 2)
    logger.log(logger.INFO, light)
end


function on_renderer_init()
    do
        local e = scene.create_actor_skinned('honoka_0', 'sungmin/honoka_basic_3.dmd')
        local t = e:get_transform_view()
        t:get_pos():set_x(-2)
        t:rotate_degree(90, 0, 1, 0)
        t:set_scale(0.3)
        e:notify_transform_change();
    end

    do
        local e = scene.create_actor_skinned('honoka_1', 'sungmin/honoka_basic_3.dmd')
        local t = e:get_transform_view()
        t:set_scale(0.3)
        e:notify_transform_change();
    end

    do
        local e = scene.create_actor_skinned('', '_asset/model/Character Running.dmd')
        local t = e:get_transform_view()
        t:get_pos():set_x(2)
        t:rotate_degree(90, 0, 1, 0);
        e:notify_transform_change();
        e:set_anim_index(0)
    end

    do
        local e = scene.create_actor_static('', '_asset/model/sponza.dmd')
        local t = e:get_transform_view()
        t:set_scale(0.01)
        t:rotate_degree(90, 1, 0, 0)
        e:notify_transform_change()
    end

    do
        local e = scene.create_actor_static('', '_asset/model/simple_box.dmd')
        local t = e:get_transform_view()
        t:get_pos():set_z(-1)
        e:notify_transform_change()
    end
end
