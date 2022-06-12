#include <algorithm>

#include <Collision/Exceptions.hpp>
#include <Collision/PolygonalCollider.hpp>
#include <Debug/Logger.hpp>
#include <Utils/VectorUtils.hpp>

namespace obe::collision
{
    bool points_comparator(const Transform::UnitVector& first, const Transform::UnitVector& second)
    {
        if (first.x < second.x)
            return true;
        if (first.x == second.x)
            return first.y > second.y;
        return false;
    }

    double points_distance(const Transform::UnitVector& first, const Transform::UnitVector& second)
    {
        return std::sqrt(std::pow(second.x - first.x, 2) + std::pow(second.y - first.y, 2));
    }

    double cross(const Transform::UnitVector& O, const Transform::UnitVector& A,
        const Transform::UnitVector& B)
    {
        return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
    }

    std::vector<Transform::UnitVector> convex_hull(std::vector<Transform::UnitVector> points)
    {
        std::ranges::sort(points, points_comparator);
        if (points.size() <= 1)
            return points;
        std::vector<Transform::UnitVector> lower_hull;
        for (Transform::UnitVector& point : points)
        {
            while (lower_hull.size() >= 2
                && cross(lower_hull[lower_hull.size() - 2], lower_hull[lower_hull.size() - 1], point)
                    <= 0)
                lower_hull.pop_back();
            lower_hull.push_back(point);
        }
        std::ranges::reverse(points);
        std::vector<Transform::UnitVector> upper_hull;
        for (Transform::UnitVector& point : points)
        {
            while (upper_hull.size() >= 2
                && cross(upper_hull[upper_hull.size() - 2], upper_hull[upper_hull.size() - 1], point)
                    <= 0)
                upper_hull.pop_back();
            upper_hull.push_back(point);
        }
        std::vector<Transform::UnitVector> full_hull;
        full_hull.reserve(lower_hull.size() + upper_hull.size() - 2);
        full_hull.insert(full_hull.end(), lower_hull.begin(), lower_hull.end() - 1);
        full_hull.insert(full_hull.end(), upper_hull.begin(), upper_hull.end() - 1);
        return full_hull;
    }

    /**
     * \brief Returns colliders that may be potentially touched (Axis-Aligned Bounding Box filtering)
    */
    std::vector<PolygonalCollider*> aabb_filter(const PolygonalCollider& coll,
        const Transform::UnitVector& offset, const std::vector<PolygonalCollider*>& colliders)
    {
        const Transform::Rect bounding_box = coll.get_bounding_box();
        Transform::Rect aabb;
        aabb.setSize(bounding_box.getSize() + Transform::UnitVector(abs(offset.x), abs(offset.y)));
        if (offset.x >= 0 && offset.y >= 0)
        {
            aabb.setPosition(bounding_box.getPosition(Transform::Referential::TopLeft),
                Transform::Referential::TopLeft);
        }
        else if (offset.x >= 0 && offset.y < 0)
        {
            aabb.setPosition(bounding_box.getPosition(Transform::Referential::BottomLeft),
                Transform::Referential::BottomLeft);
        }
        else if (offset.x < 0 && offset.y >= 0)
        {
            aabb.setPosition(bounding_box.getPosition(Transform::Referential::TopRight),
                Transform::Referential::TopRight);
        }
        else // offset.x < 0 && offset.y < 0
        {
            aabb.setPosition(bounding_box.getPosition(Transform::Referential::BottomRight),
                Transform::Referential::BottomRight);
        }
        std::vector<PolygonalCollider*> colliders_to_check;
        for (auto& collider : colliders)
        {
            if (collider != &coll && aabb.intersects(collider->get_bounding_box()))
                colliders_to_check.push_back(collider);
        }
        return colliders_to_check;
    }

    PolygonalCollider::PolygonalCollider(const std::string& id)
        : Selectable(false)
        , Component(id)
    {
    }

    PolygonalCollider::PolygonalCollider(const PolygonalCollider& collider)
        : Selectable(false)
        , Component(collider.getId())
        , Polygon(collider)
        , m_parent_id(collider.m_parent_id)
        , m_tags(collider.m_tags)
    {
        m_angle = collider.m_angle;
        m_unit = collider.m_unit;
        m_position = collider.m_position;
    }

    PolygonalCollider& PolygonalCollider::operator=(const PolygonalCollider& collider)
    {
        m_angle = collider.m_angle;
        m_position = collider.m_position;
        m_unit = collider.m_unit;
        size_t index = 0;
        for (const auto& point : collider.m_points)
        {
            m_points.push_back(std::make_unique<Transform::PolygonPoint>(
                *this, index++, Transform::UnitVector(point->x, point->y, point->unit)));
        }

        m_parent_id = collider.m_parent_id;
        m_tags = collider.m_tags;
        m_angle = collider.m_angle;
        m_unit = collider.m_unit;
        m_position = collider.m_position;

        return *this;
    }

    std::string_view PolygonalCollider::type() const
    {
        return ComponentType;
    }

    Transform::Rect PolygonalCollider::get_bounding_box() const
    {
        if (m_update_bounding_box)
        {
            m_bounding_box = Transform::Polygon::get_bounding_box();
            m_update_bounding_box = false;
        }
        return m_bounding_box;
    }

    void PolygonalCollider::add_point(const Transform::UnitVector& position, int point_index)
    {
        Transform::Polygon::add_point(position, point_index);
        m_update_bounding_box = true;
    }

    void PolygonalCollider::move(const Transform::UnitVector& position)
    {
        Transform::Polygon::move(position);
        m_bounding_box.move(position);
    }

    void PolygonalCollider::rotate(float angle, Transform::UnitVector origin)
    {
        Transform::Polygon::rotate(angle, origin);
        m_update_bounding_box = true;
    }

    void PolygonalCollider::set_position(const Transform::UnitVector& position)
    {
        const Transform::UnitVector diff = position - this->getPosition();
        Transform::Polygon::set_position(position);
        m_bounding_box.move(diff);
    }

    void PolygonalCollider::set_rotation(float angle, Transform::UnitVector origin)
    {
        Transform::Polygon::set_rotation(angle, origin);
        m_update_bounding_box = true;
    }

    void PolygonalCollider::set_position_from_centroid(const Transform::UnitVector& position)
    {
        const Transform::UnitVector old_position = this->getPosition();
        Transform::Polygon::set_position_from_centroid(position);
        m_bounding_box.move(old_position - this->getPosition());
    }

    void PolygonalCollider::reset_unit(Transform::Units unit)
    {
    }

    CollisionData PolygonalCollider::get_distance_before_collision(
        const Transform::UnitVector& offset) const
    {
        std::vector<std::pair<PolygonalCollider*, Transform::UnitVector>> reachable_colliders;

        CollisionData collision_data;
        collision_data.offset = offset;

        std::vector<PolygonalCollider*> colliders_to_check = aabb_filter(*this, offset, Pool);

        for (auto& collider : colliders_to_check)
        {
            if (check_tags(*collider))
            {
                const Transform::UnitVector max_distance
                    = this->get_distance_before_collision(*collider, offset, false);
                if (max_distance != offset)
                    reachable_colliders.emplace_back(collider, max_distance);
            }
        }

        if (!reachable_colliders.empty())
        {
            // Get lowest offset between this collider and a reachable collider
            for (auto& reachable : reachable_colliders)
            {
                if (reachable.second.magnitude()
                    < collision_data.offset.to(Transform::Units::ScenePixels).magnitude())
                {
                    collision_data.offset = reachable.second;
                }
            }

            // Get touched colliders (=> in range of lowest offset)
            for (auto& reachable : reachable_colliders)
            {
                if (reachable.second == collision_data.offset)
                {
                    collision_data.colliders.push_back(reachable.first);
                }
            }
        }

        return collision_data;
    }

    std::string PolygonalCollider::get_parent_id() const
    {
        return m_parent_id;
    }

    vili::node PolygonalCollider::schema() const
    {
        return vili::object {};
    }

    void PolygonalCollider::set_parent_id(const std::string& parent)
    {
        m_parent_id = parent;
    }

    void PolygonalCollider::add_tag(ColliderTagType tag_type, const std::string& tag)
    {
        if (!Utils::Vector::contains(tag, m_tags.at(tag_type)))
            m_tags.at(tag_type).push_back(tag);
        else
            debug::Log->warn("<PolygonalCollider> Tag '{0}' is already in "
                             "PolygonalCollider '{1}'",
                tag, m_id);
    }

    void PolygonalCollider::clear_tags(ColliderTagType tag_type)
    {
        m_tags.at(tag_type).clear();
    }

    CollisionData PolygonalCollider::does_collide(const Transform::UnitVector& offset) const
    {
        CollisionData collision_data;
        collision_data.offset = offset;

        const std::vector<PolygonalCollider*> colliders_to_check = aabb_filter(*this, offset, Pool);

        for (auto& collider : colliders_to_check)
        {
            if (check_tags(*collider))
            {
                if (this->does_collide(*collider, offset, false))
                {
                    collision_data.colliders.push_back(collider);
                }
            }
        }

        return collision_data;
    }

    void PolygonalCollider::remove_tag(ColliderTagType tag_type, const std::string& tag)
    {
        std::vector<std::string>& tags = m_tags.at(tag_type);
        std::erase(tags, tag);
    }

    bool PolygonalCollider::contains_tag(ColliderTagType tag_type, const std::string& tag) const
    {
        return Utils::Vector::contains(tag, m_tags.at(tag_type));
    }

    bool PolygonalCollider::matches_any_tag(
        ColliderTagType tag_type, const std::vector<std::string>& tags) const
    {
        if (m_tags.empty())
            return false;
        for (const std::string& tag : tags)
        {
            if (Utils::Vector::contains(tag, m_tags.at(tag_type)))
                return true;
        }
        return false;
    }

    std::vector<std::string> PolygonalCollider::get_all_tags(ColliderTagType tag_type) const
    {
        return m_tags.at(tag_type);
    }

    Transform::UnitVector PolygonalCollider::get_distance_before_collision(
        PolygonalCollider& collider, const Transform::UnitVector& offset,
        const bool perform_aabb_filter) const
    {
        constexpr Transform::Units px_unit = Transform::Units::ScenePixels;
        const Transform::UnitVector t_offset = offset.to(px_unit);
        if (perform_aabb_filter && aabb_filter(*this, offset, { &collider }).empty())
            return offset;
        bool in_front = false;
        Transform::UnitVector min_dep;
        const auto calc_min_distance_dep
            = [this](const Transform::PolygonPath& sol1, const Transform::PolygonPath& sol2,
                  const Transform::UnitVector& t_offset)
            -> std::tuple<double, Transform::UnitVector, bool>
        {
            double min_distance = -1;
            bool in_front = false;

            constexpr Transform::Units px_unit = Transform::Units::ScenePixels;
            Transform::UnitVector min_displacement(px_unit);
            Transform::UnitVector point1(px_unit);
            Transform::UnitVector point2(px_unit);
            Transform::UnitVector point3(px_unit);
            Transform::UnitVector s1(px_unit);
            for (auto& current_point : sol1)
            {
                const Transform::UnitVector point0 = current_point->to(px_unit);
                for (int i = 0; i < sol2.size(); i++)
                {
                    point1 = point0 + t_offset;
                    point2 = sol2[i]->to(px_unit);
                    point3 = sol2[(i == sol2.size() - 1) ? 0 : i + 1]->to(px_unit);

                    s1 = point1 - point0;
                    const Transform::UnitVector s2 = point3 - point2;

                    const double s = (-s1.y * (point0.x - point2.x) + s1.x * (point0.y - point2.y))
                        / (-s2.x * s1.y + s1.x * s2.y);
                    const double t = (s2.x * (point0.y - point2.y) - s2.y * (point0.x - point2.x))
                        / (-s2.x * s1.y + s1.x * s2.y);

                    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
                    {
                        in_front = true;
                        const Transform::UnitVector ip
                            = point0 + (s1 * Transform::UnitVector(t, t, s1.unit));

                        const double distance = std::sqrt(
                            std::pow((point0.x - ip.x), 2) + std::pow((point0.y - ip.y), 2));
                        if (distance < min_distance || min_distance == -1)
                        {
                            min_distance = distance;
                            const double x_comp = t * s1.x;
                            const double y_comp = t * s1.y;
                            min_displacement.set((x_comp > 0) ? std::floor(x_comp) : std::ceil(x_comp),
                                (y_comp > 0) ? std::floor(y_comp) : std::ceil(y_comp));
                        }
                    }
                }
            }
            return std::make_tuple(min_distance, min_displacement, in_front);
        };
        const Transform::PolygonPath& f_path = m_points;
        const Transform::PolygonPath& s_path = collider.getAllPoints();

        const auto tdm1 = calc_min_distance_dep(f_path, s_path, t_offset);
        auto tdm2 = calc_min_distance_dep(
            s_path, f_path, t_offset * Transform::UnitVector(-1.0, -1.0, t_offset.unit));
        std::get<1>(tdm2).x = -std::get<1>(tdm2).x;
        std::get<1>(tdm2).y = -std::get<1>(tdm2).y;
        if (std::get<2>(tdm1) || std::get<2>(tdm2))
            in_front = true;

        if (!in_front)
            min_dep = t_offset;
        else if (std::get<0>(tdm1) == 0 || std::get<0>(tdm2) == 0)
            return Transform::UnitVector(0, 0, t_offset.unit);
        else if (std::get<0>(tdm1) > 0
            && (std::get<0>(tdm1) <= std::get<0>(tdm2) || std::get<0>(tdm2) == -1))
            min_dep = std::get<1>(tdm1);
        else if (std::get<0>(tdm2) > 0)
            min_dep = std::get<1>(tdm2);
        else
            return Transform::UnitVector(0, 0, t_offset.unit);

        return min_dep;
    }

    bool PolygonalCollider::does_collide(PolygonalCollider& collider,
        const Transform::UnitVector& offset, const bool perform_aabb_filter) const
    {
        if (perform_aabb_filter && aabb_filter(*this, offset, { &collider }).empty())
            return false;
        std::vector<Transform::UnitVector> p_set1;
        p_set1.reserve(m_points.size());
        std::vector<Transform::UnitVector> p_set2;
        p_set2.reserve(m_points.size());

        for (const auto& point : m_points)
            p_set1.push_back(*point);
        for (const auto& point : collider.getAllPoints())
            p_set2.push_back(*point);
        for (auto& apply_offset : p_set1)
            apply_offset += offset;
        constexpr auto point_in_polygon = [](const std::vector<Transform::UnitVector>& poly,
            const Transform::UnitVector& p_test) -> bool
        {
            int i, j, c = 0;
            const int n_pt = static_cast<int>(poly.size());
            for (i = 0, j = n_pt - 1; i < n_pt; j = i++)
            {
                if (((poly[i].y > p_test.y) != (poly[j].y > p_test.y))
                    && (p_test.x
                        < (poly[j].x - poly[i].x) * (p_test.y - poly[i].y) / (poly[j].y - poly[i].y)
                            + poly[i].x))
                    c = !c;
            }
            return c;
        };
        for (Transform::UnitVector& p_test : p_set1)
        {
            if (point_in_polygon(p_set2, p_test))
                return true;
        }
        return std::ranges::any_of(
            p_set2, [&p_set1](const auto& p_test) { return point_in_polygon(p_set1, p_test); });
    }

    vili::node PolygonalCollider::dump() const
    {
        vili::node result;
        result["unit"] = Transform::UnitsMeta::toString(m_unit);
        result["points"] = vili::array {};
        for (auto& point : m_points)
        {
            const Transform::UnitVector point_coordinates = point->to(m_unit);
            result["points"].push(vili::object { { "x", point_coordinates.x }, { "y", point_coordinates.y } });
        }
        return result;
    }

    void PolygonalCollider::load(const vili::node& data)
    {
        auto add_tag_helper = [this](ColliderTagType type, const vili::node& tag)
        {
            if (tag.is<vili::string>())
            {
                this->add_tag(type, tag);
            }
            else if (tag.is<vili::array>())
            {
                for (const vili::node& item : tag)
                {
                    if (item.is<vili::string>())
                    {
                        this->add_tag(type, item);
                    }
                    else
                    {
                        throw Exceptions::InvalidTagFormat(m_id, ColliderTagTypeMeta::toString(type),
                            vili::to_string(item.type()), EXC_INFO);
                    }
                }
            }
            else
            {
                throw Exceptions::InvalidTagFormat(
                    m_id, ColliderTagTypeMeta::toString(type), vili::to_string(tag.type()), EXC_INFO);
            }
        };
        const Transform::Units points_unit = Transform::UnitsMeta::fromString(data.at("unit"));
        for (const vili::node& collider_point : data.at("points"))
        {
            const Transform::UnitVector point_coordinates
                = Transform::UnitVector(collider_point["x"], collider_point["y"], points_unit);
            this->add_point(point_coordinates);
        }
        this->setWorkingUnit(points_unit);

        if (data.contains("tag"))
        {
            add_tag_helper(ColliderTagType::Tag, data["tag"]);
        }
        if (data.contains("accept"))
        {
            add_tag_helper(ColliderTagType::Accepted, data["accept"]);
        }
        if (data.contains("reject"))
        {
            add_tag_helper(ColliderTagType::Rejected, data["reject"]);
        }
    }

    bool PolygonalCollider::check_tags(const PolygonalCollider& collider) const
    {
        if (this->matches_any_tag(
                ColliderTagType::Rejected, collider.get_all_tags(ColliderTagType::Tag)))
            return false;
        if (!m_tags.at(ColliderTagType::Accepted).empty()
            && !this->matches_any_tag(
                ColliderTagType::Accepted, collider.get_all_tags(ColliderTagType::Tag)))
            return false;
        return true;
    }
} // namespace obe::collision
