#pragma once

#include "d_model_data.h"


namespace dal {

    class IMeshGenerator {

    public:
        virtual ~IMeshGenerator() = default;

        virtual void generate_points(Mesh<VertexStatic>& output) const = 0;

        Mesh<VertexStatic> generate_points() const;

    };

}
