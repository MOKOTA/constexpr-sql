#pragma once

#include <fstream>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "cexpr/string.hpp"

#include "sql/column.hpp"
#include "sql/index.hpp"
#include "sql/row.hpp"

namespace sql
{

	template <cexpr::string Name, typename Index, typename... Cols>
	class schema
	{
	public:
		static constexpr auto name{ Name };

		using row_type = sql::variadic_row<Cols...>::row_type;
		using container = typename
			std::conditional_t<
				std::is_same_v<Index, sql::index<>>,
				std::vector<row_type>,
				std::multiset<row_type, typename Index::template comparator<row_type>>
			>;
		using const_iterator = typename container::const_iterator;
		
		schema() = default;

		template <typename Type, typename... Types>
		schema(std::vector<Type> const& col, Types const&... cols) : schema{}
		{
			insert(col, cols...);
		}

		template <typename Type, typename... Types>
		schema(std::vector<Type>&& col, Types&&... cols) : schema{}
		{
			insert(std::forward<Type>(col), std::forward<Types>(cols)...);
		}

		template <typename... Types>
		inline void emplace(Types const&... vals)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.emplace_back(vals...);
			}
			else
			{
				table_.emplace(vals...);
			}
		}

		template <typename... Types>
		inline void emplace(Types&&... vals)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.emplace_back(vals...);
			}
			else
			{
				table_.emplace(vals...);
			}
		}

		template <typename Type, typename... Types>
		void insert(std::vector<Type> const& col, Types const&... cols)
		{
			for (std::size_t i{}; i < col.size(); ++i)
			{
				emplace(col[i], cols[i]...);
			}
		}

		template <typename Type, typename... Types>
		void insert(std::vector<Type>&& col, Types&&... cols)
		{
			for (std::size_t i{}; i < col.size(); ++i)
			{
				emplace(std::forward<Type>(col[i]),std::forward<Types>(cols[i])...);
			}
		}

		void insert(row_type const& row)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.push_back(row);
			}
			else
			{
				table_.insert(row);
			}
		}

		void insert(row_type&& row)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.push_back(std::forward<row_type>(row));
			}
			else
			{
				table_.insert(std::forward<row_type>(row));
			}
		}

		inline const_iterator begin() const
		{
			return table_.begin();
		}

		inline const_iterator end() const
		{
			return table_.end();
		}

	private:
		container table_;
	};

	namespace
	{

		template <typename Row, char Delim>
		void fill(std::fstream& file, Row& row)
		{
			if constexpr (!std::is_same_v<Row, sql::void_row>)
			{
				if constexpr (std::is_same_v<typename Row::column::type, std::string>)
				{
					if constexpr (std::is_same_v<typename Row::next, sql::void_row>)
					{
						std::getline(file, row.head());
					}
					else
					{
						std::getline(file, row.head(), Delim);
					}
				}
				else
				{
					file >> row.head();
				}

				fill<typename Row::next, Delim>(file, row.tail());
			}
		}

	} // namespace

	// helper function for users to load a data into a schema from a file
	template <typename Schema, char Delim>
	Schema load(std::string const& data)
	{
		auto file{ std::fstream(data) };
		Schema table{};
		typename Schema::row_type row{};

		while (file)
		{
			fill<typename Schema::row_type, Delim>(file, row);
			table.insert(std::move(row));

			// in case last stream extraction did not remove newline
			if (file.get() != '\n')
			{
				file.unget();
			}
		}

		return table;
	}

} // namespace sql
