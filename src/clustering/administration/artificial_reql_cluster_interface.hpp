// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ARTIFICIAL_REQL_CLUSTER_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_ARTIFICIAL_REQL_CLUSTER_INTERFACE_HPP_

#include <map>
#include <set>
#include <string>

#include "clustering/administration/cluster_config.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/servers/server_config.hpp"
#include "clustering/administration/servers/server_status.hpp"
#include "clustering/administration/stats/debug_stats_backend.hpp"
#include "clustering/administration/stats/stats_backend.hpp"
#include "clustering/administration/tables/db_config.hpp"
#include "clustering/administration/tables/debug_table_status.hpp"
#include "clustering/administration/tables/table_config.hpp"
#include "clustering/administration/tables/table_status.hpp"
#include "clustering/administration/issues/issues_backend.hpp"
#include "clustering/administration/auth/permissions_artificial_table_backend.hpp"
#include "clustering/administration/auth/users_artificial_table_backend.hpp"
#include "clustering/administration/logs/logs_backend.hpp"
#include "clustering/administration/jobs/backend.hpp"
#include "containers/name_string.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/artificial_table/in_memory.hpp"
#include "rdb_protocol/context.hpp"

class namespace_repo_t;
class real_reql_cluster_interface_t;
class server_config_client_t;
class table_meta_client_t;

/* The `artificial_reql_cluster_interface_t` is responsible for handling queries to the
`rethinkdb` database. It's implemented as a proxy over the
`real_reql_cluster_interface_t`; queries go first to the `artificial_...`, and if they
aren't related to the `rethinkdb` database, they get passed on to the `real_...`. */

class artificial_reql_cluster_interface_t : public reql_cluster_interface_t {
public:
    artificial_reql_cluster_interface_t(
            /* This is the name of the special database; i.e. `rethinkdb` */
            name_string_t database,
            /* These are the tables that live in the special database. For each pair, the
            first value will be used if `identifier_format` is unspecified or "name", and
            the second value will be used if `identifier_format` is "uuid". */
            const std::map<name_string_t,
                std::pair<artificial_table_backend_t *, artificial_table_backend_t *>
                > &tables,
            /* This is the `real_reql_cluster_interface_t` that we're proxying. */
            reql_cluster_interface_t *next) :
        m_database(database),
        m_tables(tables),
        m_next(next) { }

    bool db_create(
            auth::user_context_t const &user_context,
            const name_string_t &name,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);
    bool db_drop(
            auth::user_context_t const &user_context,
            const name_string_t &name,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);
    bool db_list(
            signal_t *interruptor,
            std::set<name_string_t> *names_out, admin_err_t *error_out);
    bool db_find(const name_string_t &name,
            signal_t *interruptor,
            counted_t<const ql::db_t> *db_out, admin_err_t *error_out);
    bool db_config(
            auth::user_context_t const &user_context,
            const counted_t<const ql::db_t> &db,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out,
            admin_err_t *error_out);

    bool table_create(
            auth::user_context_t const &user_context,
            const name_string_t &name,
            counted_t<const ql::db_t> db,
            const table_generate_config_params_t &config_params,
            const std::string &primary_key,
            write_durability_t durability,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);
    bool table_drop(
            auth::user_context_t const &user_context,
            const name_string_t &name,
            counted_t<const ql::db_t> db,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);
    bool table_list(counted_t<const ql::db_t> db,
            signal_t *interruptor,
            std::set<name_string_t> *names_out, admin_err_t *error_out);
    bool table_find(const name_string_t &name, counted_t<const ql::db_t> db,
            boost::optional<admin_identifier_format_t> identifier_format,
            signal_t *interruptor, counted_t<base_table_t> *table_out,
            admin_err_t *error_out);
    bool table_estimate_doc_counts(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::env_t *env,
            std::vector<int64_t> *doc_counts_out,
            admin_err_t *error_out);
    bool table_config(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out,
            admin_err_t *error_out);
    bool table_status(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out,
            admin_err_t *error_out);

    bool table_wait(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            table_readiness_t readiness,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);
    bool db_wait(
            counted_t<const ql::db_t> db,
            table_readiness_t readiness,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);

    bool table_reconfigure(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            const table_generate_config_params_t &params,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);
    bool db_reconfigure(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const table_generate_config_params_t &params,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);

    bool table_emergency_repair(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            emergency_repair_mode_t,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);

    bool table_rebalance(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);
    bool db_rebalance(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);

    bool grant_global(
            auth::user_context_t const &user_context,
            auth::username_t username,
            ql::datum_t permissions,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);
    bool grant_database(
            auth::user_context_t const &user_context,
            database_id_t const &database_id,
            auth::username_t username,
            ql::datum_t permissions,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);
    bool grant_table(
            auth::user_context_t const &user_context,
            database_id_t const &database_id,
            namespace_id_t const &table_id,
            auth::username_t username,
            ql::datum_t permissions,
            signal_t *interruptor,
            ql::datum_t *result_out,
            admin_err_t *error_out);

    bool modifier_create(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const modifier_config_t &config,
            signal_t *interruptor,
            admin_err_t *error_out);
    bool modifier_drop(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            signal_t *interruptor,
            admin_err_t *error_out);

    bool sindex_create(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const std::string &name,
            const sindex_config_t &config,
            signal_t *interruptor,
            admin_err_t *error_out);
    bool sindex_drop(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const std::string &name,
            signal_t *interruptor,
            admin_err_t *error_out);
    bool sindex_rename(
            auth::user_context_t const &user_context,
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const std::string &name,
            const std::string &new_name,
            bool overwrite,
            signal_t *interruptor,
            admin_err_t *error_out);
    bool sindex_list(
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            signal_t *interruptor,
            admin_err_t *error_out,
            std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
                *configs_and_statuses_out);

private:
    name_string_t m_database;
    std::map<name_string_t,
        std::pair<artificial_table_backend_t *, artificial_table_backend_t *> > m_tables;
    reql_cluster_interface_t *m_next;
};

/* `admin_artificial_tables_t` constructs the `artificial_reql_cluster_interface_t` along
with all of the tables that will go in it. */

class admin_artificial_tables_t {
public:
    admin_artificial_tables_t(
            real_reql_cluster_interface_t *_next_reql_cluster_interface,
            boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
                auth_semilattice_view,
            boost::shared_ptr<semilattice_readwrite_view_t<
                cluster_semilattice_metadata_t> > _semilattice_view,
            boost::shared_ptr<semilattice_readwrite_view_t<
                heartbeat_semilattice_metadata_t> > _heartbeat_view,
            clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
                cluster_directory_metadata_t> > > _directory_view,
            watchable_map_t<peer_id_t, cluster_directory_metadata_t>
                *_directory_map_view,
            table_meta_client_t *_table_meta_client,
            server_config_client_t *_server_config_client,
            namespace_repo_t *_namespace_repo,
            mailbox_manager_t *_mailbox_manager);
    reql_cluster_interface_t *get_reql_cluster_interface() {
        return reql_cluster_interface.get();
    }

    /* These variables exist only to manage the lifetimes of the various backends; they
    are initialized in the constructor and then passed to the `reql_cluster_interface`,
    but not used after that.

    The arrays of two backends are used when the contents of the table depends on the
    identifier format; one backend uses names and the other uses UUIDs. */

    scoped_ptr_t<auth::permissions_artificial_table_backend_t> permissions_backend[2];
    scoped_ptr_t<auth::users_artificial_table_backend_t> users_backend;
    scoped_ptr_t<cluster_config_artificial_table_backend_t> cluster_config_backend;
    scoped_ptr_t<db_config_artificial_table_backend_t> db_config_backend;
    scoped_ptr_t<issues_artificial_table_backend_t> issues_backend[2];
    scoped_ptr_t<logs_artificial_table_backend_t> logs_backend[2];
    scoped_ptr_t<server_config_artificial_table_backend_t> server_config_backend;
    scoped_ptr_t<server_status_artificial_table_backend_t> server_status_backend[2];
    scoped_ptr_t<stats_artificial_table_backend_t> stats_backend[2];
    scoped_ptr_t<table_config_artificial_table_backend_t> table_config_backend[2];
    scoped_ptr_t<table_status_artificial_table_backend_t> table_status_backend[2];

    scoped_ptr_t<in_memory_artificial_table_backend_t> debug_scratch_backend;
    scoped_ptr_t<debug_stats_artificial_table_backend_t> debug_stats_backend;
    scoped_ptr_t<debug_table_status_artificial_table_backend_t>
        debug_table_status_backend;

    scoped_ptr_t<artificial_reql_cluster_interface_t> reql_cluster_interface;
    scoped_ptr_t<jobs_artificial_table_backend_t> jobs_backend[2];
};

#endif /* CLUSTERING_ADMINISTRATION_ARTIFICIAL_REQL_CLUSTER_INTERFACE_HPP_ */

