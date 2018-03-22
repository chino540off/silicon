#pragma once

# include <cassert>
# include <deque>
# include <mutex>
# include <memory>

# include <mongocxx/client.hpp>
# include <mongocxx/instance.hpp>
# include <mongocxx/logger.hpp>

# include <silicon/error.hh>

namespace sl {
namespace mw {
namespace mongodb {

struct logger:
  public mongocxx::logger
{
  virtual void operator()(mongocxx::log_level l,
                          bsoncxx::stdx::string_view d,
                          bsoncxx::stdx::string_view m) noexcept
  {
    std::cout << "[" << mongocxx::to_string(l) << "]["
              << d << "]" << m << "\n";
  }
};

class pool;

class connection
{
  public:
    inline connection(std::shared_ptr<mongocxx::client> c,
                      std::shared_ptr<pool> pool);

  public:
    auto & operator->()
    {
      return c_;
    }

  private:
    std::shared_ptr<mongocxx::client> c_ = nullptr;
    std::shared_ptr<pool> pool_ = nullptr;

    std::shared_ptr<int> guard_;
};

class pool:
  public std::enable_shared_from_this<pool>
{
  public:
    pool(std::string const & uri):
      uri_(uri)
    {
    }

  public:
    connection acquire()
    {
      std::shared_ptr<mongocxx::client> c = nullptr;

      {
        std::unique_lock<std::mutex> l(mutex_);

        if (!pool_.empty())
        {
          c = pool_.back();
          pool_.pop_back();
        }
      }

      if (!c)
      {
        c = std::make_shared<mongocxx::client>(uri_);

        if (!c)
          throw error::internal_server_error("Cannot connect to the database");
      }

      assert(c);
      return connection(c, shared_from_this());
    }

    void release(std::shared_ptr<mongocxx::client> c)
    {
      std::unique_lock<std::mutex> l(mutex_);
      pool_.push_back(c);
    }

  private:
    mongocxx::uri uri_;

    std::mutex mutex_;
    std::deque<std::shared_ptr<mongocxx::client>> pool_;
};

connection::connection(std::shared_ptr<mongocxx::client> c,
                       std::shared_ptr<pool> pool):
  c_(c),
  pool_(pool)
{
  guard_ = std::shared_ptr<int>((int *)42, [pool, c] (int *)
                               {
                                 pool->release(c);
                               });
}

class connection_factory
{
  public:
    template <typename... O>
    connection_factory(std::string const & uri, O&&...):
      inst_(std::make_shared<mongocxx::instance>(std::make_unique<logger>())),
      pool_(std::make_shared<pool>(uri))
    {
    }

    connection instantiate()
    {
      return pool_->acquire();
    }

  private:
    std::shared_ptr<mongocxx::instance> inst_ = nullptr;
    std::shared_ptr<pool> pool_ = nullptr;
};

} /** !mongodb */
} /** !mw */
} /** !sl */
