scene = require('scene')
console = require('console')


function on_renderer_init(a)
    local honoka_0 = scene.create_actor_skinned('honoka_0', 'sungmin/honoka_basic_3.dmd')
    local t = scene.get_transform_view_of(honoka_0)
    t:set_pos_x(-2)
    t:rotate_degree(90, 0, 1, 0)
    t:set_scale(0.3)

    local honoka_1 = scene.create_actor_skinned('honoka_1', 'sungmin/honoka_basic_3.dmd')
    local t = scene.get_transform_view_of(honoka_1)
    t:set_scale(0.3)
end
