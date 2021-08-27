scene = require('scene')
console = require('console')


function on_renderer_init(a)
    do
        local e = scene.create_actor_skinned('honoka_0', 'sungmin/honoka_basic_3.dmd')
        local t = e:get_transform_view()
        t:set_pos_x(-2)
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
        t:set_pos_x(2)
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
        t:set_pos_z(-1)
        e:notify_transform_change()
    end
end
