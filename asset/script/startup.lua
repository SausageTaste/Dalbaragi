scene = require('scene')
console = require('console')


function on_renderer_init(a)
    local e = scene.create_actor_skinned('honoka_0', 'sungmin/honoka_basic_3.dmd')
    local t = e:get_transform_view()
    t:set_pos_x(-2)
    t:rotate_degree(90, 0, 1, 0)
    t:set_scale(0.3)
    e:notify_transform_change();

    local e = scene.create_actor_skinned('honoka_1', 'sungmin/honoka_basic_3.dmd')
    local t = e:get_transform_view()
    t:set_scale(0.3)
    e:notify_transform_change();
end
