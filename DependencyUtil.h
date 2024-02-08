#pragma once

#include <vector>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/graph_utility.hpp>

using std::vector;


namespace Jikes { // Open namespace Jikes block


	namespace PathModel {

template <typename Tchar>
class BaseDependencyUtil
{
public:



	typedef std::basic_string<Tchar> Tstring;
	typedef std::pair< Tstring, Tstring> Edge;
	typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::bidirectionalS> Graph;
	typedef boost::graph_traits< boost::adjacency_list< boost::vecS,
	boost::vecS, boost::directedS> >::vertex_descriptor Vertex;

	class cycleArray
	{
	public:
		vector< Tstring > _array;
		 Tstring to_string() const
		{
			Tstring temp;
			size_t size = _array.size();
			for (size_t i = 0; i < size; ++i)
			{
				temp += _array[i];
				temp += Tchar('-');
				temp += Tchar('-');
				temp += Tchar('>');
			}
			if(size)
				temp += _array[0];

			return temp;
		}
	};

	BaseDependencyUtil():_consistency(false), _is_DAG(false){};
	~BaseDependencyUtil(){};


	void AddOneDependency(const Edge& e)
	{
		
		
		const auto& first_string = e.first;
		const auto& second_string = e.second;
		int index_first = -1, index_second = -1;
		size_t name_size = name.size();
		for (size_t k = 0; k < name_size; ++k) {
			if (name[k] == first_string)
			{
				index_first = k;
			}
			else if (name[k] == second_string)
			{
				index_second = k;
			}
		}


		if (-1 == index_first)
		{
			name.push_back(first_string);
			index_first = name.size() - 1;
			
		}
		if (-1 == index_second)
		{
			if (second_string == first_string)
				return;

			name.push_back(second_string);
			index_second = name.size() - 1;
			
		}
		_consistency = false;
		dependencies_info.push_back(e);
		
		add_edge(index_first, index_second, G);
	

	}
	void AddDependencies(std::vector< Edge >& dependencies)
	{
		size_t size = dependencies.size();
		for (size_t i = 0; i < size; ++i){
			AddOneDependency(dependencies[i]);
		}

	}

	// 如何判断是否有环，如果make_sort_result.size() 为 0 ,意味着有环，这个时候如果需要得知环，可以调用getCycles函数来获取
	const vector< Tstring >& getSort() {

		if (_consistency)// 图没有被改变，而且上一次已经进行过拓扑排序了
		{
			return make_sort_result;
		}

		Topological_sort();	
		return make_sort_result;
	}

	// 必须先CaculateCycles
	const vector < cycleArray >& getCycles()
	{

		if (!_consistency)// 图被改变，而且上一次已经没有过拓扑排序了
		{
			getSort();
		}
	
		if (_is_DAG)// 查看 是否是DAG，如果为真，则没有环，不需要计算了
			return _Cycles;

		if (_Cycles.size())// 已经计算过了。
			return _Cycles;
		else
		{
			CaculateCycles();
			return _Cycles;
		}
		
	}


private:
	void Topological_sort()
	{
		using namespace boost;
		make_order.clear();
		make_sort_result.clear();
		_Cycles.clear();
		_consistency = true;
		_is_DAG = true;


		MakeOrder::iterator i;
		try
		{
			topological_sort(G, std::front_inserter(make_order));
			 
			for (i = make_order.begin();
			i != make_order.end(); ++i) {
				make_sort_result.push_back(name[*i]);
			}
		}
		catch (not_a_dag& )
		{
			_is_DAG = false;
		}
	}
	void CaculateCycles()
	{
		using namespace boost;
		std::vector<int> component(num_vertices(G)), discover_time(num_vertices(G));
		std::vector<default_color_type> color(num_vertices(G));
		std::vector<Vertex> root(num_vertices(G));
		int num = strong_components(G, make_iterator_property_map(component.begin(), get(vertex_index, G)),
			root_map(make_iterator_property_map(root.begin(), get(vertex_index, G))).
			color_map(make_iterator_property_map(color.begin(), get(vertex_index, G))).
			discover_time_map(make_iterator_property_map(discover_time.begin(), get(vertex_index, G))));


		std::map <int, vector< pair<int, Tstring > > > loops;

		for (std::vector<int>::size_type i = 0; i != component.size(); ++i)
			loops[component[i]].push_back(pair<int, Tstring >(discover_time[i], name[i]));

		auto it = loops.begin();
		auto itEnd = loops.end();
		_Cycles.clear();
		for (; it != itEnd; ++it) {

			if (it->second.size() > 1)
			{


				std::sort(it->second.begin(), it->second.end(),
					[](const pair<int, Tstring >& elem1, const pair<int, Tstring >& elem2)
				{
					if (elem1.first > elem2.first)
					{
						return false;
					}
					else
					{
						return true;
					}
				});

				cycleArray one_cycle;
				one_cycle._array.reserve(it->second.size());
				for (size_t i = 0; i < it->second.size(); ++i)
				{
					one_cycle._array.push_back(it->second[i].second);
				}

				_Cycles.push_back(one_cycle);
			}
		}
	}


	bool _consistency;//有新的边添加到图中，而且没有被计算过，这个值为fasle
	bool _is_DAG;//表示图是否有环

	Graph G;

	std::vector< Tstring > name;
	typedef std::list<Vertex> MakeOrder;
	MakeOrder make_order;


	std::vector< Edge > dependencies_info;
	vector< Tstring > make_sort_result;
	vector < cycleArray > _Cycles;
};

typedef BaseDependencyUtil<char> A_DependencyUtil;
typedef BaseDependencyUtil<wchar_t> W_DependencyUtil;


}// Close namespace PathModel block


} // Close namespace Jikes block

