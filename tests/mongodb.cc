#include <silicon/api.hh>
#include <silicon/middleware_factories.hh>
#include <silicon/backends/mhd.hh>
#include <silicon/middlewares/mongodb_connection.hh>
#include <silicon/middlewares/mongodb_orm.hh>
#include "symbols.hh"

typedef decltype(iod::D(s::__id(s::_index) = std::string(),
                        s::_name(s::_index) = std::string(),
                        s::_age(s::_index) = int(),
                        s::_address = std::string(),
                        s::_city = std::string())) user;

typedef decltype(iod::D(s::__id(s::_index) = std::string(),
                        s::_name = std::string())) city;
int main()
{
  auto api = http_api(
    // create: POST /user
    sl::POST / s::_user * sl::post_parameters(s::_name = std::string(),
                                              s::_age = int(),
                                              s::_address = std::string(),
                                              s::_city = std::string()) =
    [] (auto p, sl::mw::mongodb::orm<user> & orm)
    {
      auto _id = orm.insert(iod::D(s::_name = p.name,
                                   s::_age = p.age,
                                   s::_address = p.address,
                                   s::_city = p.city));

      return iod::D(s::__id = _id);
    },

    // read: GET /user/[_id]
    sl::GET / s::_user / s::__id[std::string()] =
    [] (auto p, sl::mw::mongodb::orm<user> & orm)
    {
      return orm.find_by_id(p._id);
    },

    // update: POST /user/[_id]
    sl::POST / s::_user / s::__id[std::string()] * sl::post_parameters(s::_name = std::string(),
                                                                       s::_age = int(),
                                                                       s::_address = std::string(),
                                                                       s::_city = std::string()) =
    [] (auto p, sl::mw::mongodb::orm<user> & orm)
    {
      return orm.update_by_id(p._id,
                              iod::D(s::_name = p.name,
                                     s::_age = p.age,
                                     s::_address = p.address,
                                     s::_city = p.city));
    },

    // delete: DELETE /user/[_id]
    sl::DELETE / s::_user / s::__id[std::string()] =
    [] (auto p, sl::mw::mongodb::orm<user> & orm)
    {
      orm.delete_by_id(p._id);
    }
    );

  auto f = sl::middleware_factories(
      sl::mw::mongodb::connection_factory("mongodb://172.17.0.2:27017"),
      sl::mw::mongodb::orm_factory<user>("dbname", "users"),
      sl::mw::mongodb::orm_factory<city>("dbname", "cities")
    );

  sl::mhd_json_serve(api, f, 12345);//, s::_non_blocking);
}
