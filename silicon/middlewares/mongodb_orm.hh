#pragma once

# include <iostream>

# include <bsoncxx/json.hpp>
# include <bsoncxx/builder/basic/document.hpp>
# include <bsoncxx/builder/basic/kvp.hpp>
# include <bsoncxx/builder/stream/document.hpp>

# include <silicon/error.hh>
# include <silicon/symbols.hh>
# include <silicon/middlewares/utils.hh>
# include <silicon/middlewares/mongodb_connection.hh>

namespace sl {
namespace mw {
namespace mongodb {

template <typename T>
using extract_indexes_t = decltype(sl::mw::utils::extract_members_with_attribute(std::declval<T>(), s::_index));

template <typename T>
class orm
{
  public:
    orm(std::string const & db,
        std::string const & collection,
        connection & c):
      collection_(c->database(db).collection(collection))
    {
    }

  private:

    template <typename B>
    void extract_data(std::string & v, B const & e)
    {
      switch (e.type())
      {
        case bsoncxx::type::k_utf8:
          v = std::string(e.get_utf8().value.data(),
                          e.get_utf8().value.size());
          break;

        case bsoncxx::type::k_oid:
          v = e.get_oid().value.to_string();
          break;

        default:
          throw sl::error::internal_server_error();
      }
    }

    template <typename B>
    void extract_data(int64_t & v, B const & e)
    {
      v = e.get_int64().value;
    }

    template <typename B>
    void extract_data(int32_t & v, B const & e)
    {
      v = e.get_int32().value;
    }

  public:
    template <typename O>
    auto insert(O const & o)
    {
      auto builder = bsoncxx::builder::stream::document{};

      foreach(o) | [&] (auto & m)
      {
        builder << std::string(m.symbol().name()) << m.value();
      };

      auto doc = builder << bsoncxx::builder::stream::finalize;
      auto ores = collection_.insert_one(std::move(doc.view()));

      if (!ores && ores->inserted_id().type() != bsoncxx::type::k_oid)
        throw sl::error::internal_server_error();

      return ores->inserted_id().get_oid().value.to_string();
    }

    template <typename I>
    T find_by_id(I const & id)
    {
      auto ores = collection_.find_one(bsoncxx::builder::stream::document{}
                                       << "_id" << bsoncxx::oid{id}
                                       << bsoncxx::builder::stream::finalize);

      if (!ores)
        throw sl::error::not_found();

      T o{};

      foreach(o) | [&] (auto & m)
      {
        this->extract_data(m.value(), ores->view()[m.symbol().name()]);
      };

      return o;
    }

    template <typename I, typename O>
    auto update_by_id(I const & id, O const & o)
    {
      auto builder = bsoncxx::builder::stream::document{};

      foreach(o) | [&] (auto & m)
      {
        builder << std::string(m.symbol().name()) << m.value();
      };

      auto doc = builder << bsoncxx::builder::stream::finalize;
      auto ores = collection_.update_one(bsoncxx::builder::stream::document{}
                                           << "_id" << bsoncxx::oid{id}
                                           << bsoncxx::builder::stream::finalize,
                                           std::move(doc.view()));

      if (!ores || ores->modified_count() == 0)
        throw sl::error::not_found();
    }

    template <typename I>
    void delete_by_id(I const & id)
    {
      auto ores = collection_.delete_one(bsoncxx::builder::stream::document{}
                                         << "_id" << bsoncxx::oid{id}
                                         << bsoncxx::builder::stream::finalize);

      if (!ores || ores->deleted_count() == 0)
        throw sl::error::not_found();
    }

  private:
    mongocxx::collection collection_;
};

template <typename T>
class orm_factory
{
  public:
    typedef T object_type;
    typedef orm<T> instance_type;

  public:
    orm_factory(std::string const & db,
                std::string const & collection):
      db_(db),
      collection_(collection)
    {
    }

  public:
    void initialize(connection & c)
    {
      using bsoncxx::builder::basic::kvp;
      using bsoncxx::builder::basic::make_document;

      foreach(extract_indexes_t<T>()) | [&] (auto & m)
      {
        std::cout << m.symbol().name() << std::endl;
        c->database(db_)
          .collection(collection_)
          .create_index(make_document(kvp(std::string(m.symbol().name()), 1)), {});
      };
    }

    orm<T> instantiate(connection c)
    {
      return orm<T>(db_, collection_, c);
    }

  private:
    std::string db_;
    std::string collection_;
};

} /** !mongodb */
} /** !mw */
} /** !sl */
