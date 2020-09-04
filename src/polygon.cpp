#include <Rcpp.h>

#include "mapdeck_defaults.hpp"
#include "layers/layer_colours.hpp"
#include "spatialwidget/spatialwidget.hpp"

#include "geometries/coordinates/dimensions.hpp"
#include "interleave/primitives/primitives.hpp"

Rcpp::List polygon_defaults(int n) {
	return Rcpp::List::create(
		_["elevation"] = mapdeck::defaults::default_elevation(n),
		_["fill_colour"] = mapdeck::defaults::default_fill_colour(n),
		_["stroke_colour"] = mapdeck::defaults::default_stroke_colour(n)
	);
}

// [[Rcpp::export]]
Rcpp::List rcpp_polygon_geojson(
		Rcpp::DataFrame data,
		Rcpp::List params,
		Rcpp::StringVector geometry_columns,
		int digits
	) {

	int data_rows = data.nrows();

	Rcpp::List lst_defaults = polygon_defaults( data_rows );  // initialise with defaults

	std::unordered_map< std::string, std::string > polygon_colours = mapdeck::layer_colours::fill_stroke_colours;
	Rcpp::StringVector polygon_legend = mapdeck::layer_colours::fill_stroke_legend;
	Rcpp::StringVector parameter_exclusions = Rcpp::StringVector::create("legend","legend_options","palette","na_colour");

	return spatialwidget::api::create_geojson(
		data,
		params,
		lst_defaults,
		polygon_colours,
		polygon_legend,
		data_rows,
		parameter_exclusions,
		geometry_columns,
		true,  // jsonify legend
		digits
	);
}


// [[Rcpp::export]]
Rcpp::List rcpp_triangle_columnar(
		Rcpp::DataFrame data,
		Rcpp::List params,
		Rcpp::IntegerVector list_columns,      //
		Rcpp::StringVector geometry_columns,
		int digits
) {

	// any list-columns must have the same number of elements as cordinates
	// as they go through interleaving and get shuffled as per the triangulation of coords

	int data_rows = data.nrows();



	// TODO
	// call geometries:::rcpp_geometry_dimensions()
	// and the first column is the binaryStartIndices
	// which is the first 'row' where each matrix (ring) starts
	//
	// then 'binaryIndices' is a sequence 0:(n_coordinates-1)

	Rcpp::String poly_column = geometry_columns[0];
	SEXP poly = data[ poly_column.get_cstring() ];

	// remove teh poly column?
	// because it can't be used

	Rcpp::List dim = geometries::coordinates::geometry_dimensions( poly );
	Rcpp::IntegerMatrix dimensions = dim[ "dimensions" ];

	// For triangles, this won't work because we have made new coordinates because of ear-cutting
	// so we've added vertices.
	//
	// so, we need to interleave the coordinates and return the indices, then
	// we can expand the data based on those indicies
	// the do all the formatting

	//Rcpp::IntegerVector start_indices = dimensions( Rcpp::_, 0 );
	R_xlen_t n_geometries = dimensions.nrow();
	//int total_coordinates = dimensions( n_geometries - 1, 1 );
	// total_coordinates is the total number of coordinates AFTER ear-cutting


	//Rcpp::Rcout << "data_rows: " << data_rows << std::endl;
	//Rcpp::Rcout << "total_coords: " << total_coordinates << std::endl;


	// TODO:
	// this is the 'repeats' vector

	Rcpp::List tri_properties( list_columns.length() );
	for( R_xlen_t i = 0; i < list_columns.length(); ++i ) {
		R_xlen_t idx = list_columns[ i ];
		tri_properties[ i ] = data[ idx ];
	}
	Rcpp::List tri = interleave::primitives::interleave_triangle( poly, tri_properties );

	Rcpp::IntegerVector indices = tri["input_index"];

	// put the properties back onto 'data'
	Rcpp::List shuffled_properties = tri["properties"];
	//return shuffled_properties;
	for( R_xlen_t i = 0; i < list_columns.length(); ++i ) {
		R_xlen_t idx = list_columns[ i ];
		data[ idx ] = shuffled_properties[ i ];
	}

	//return data;

	// IFF any columns of 'data' are lists, where each element is a vector the same length as
	// the number of coordinates in that sfg_POLYGON, then that vector needs to be subset
	// according to `indices`, which will correctly align the value to the coordinate
	//

	int total_coordinates = indices.length();
	Rcpp::List lst_defaults = polygon_defaults( total_coordinates );  // initialise with defaults

	//Rcpp::Rcout << "total_coords: " << total_coordinates << std::endl;
	Rcpp::IntegerVector n_coordinates = tri["n_coordinates"];
	Rcpp::IntegerVector start_indices( n_coordinates.length() );
	start_indices[0] = 0;
	for( R_xlen_t i = 1; i < n_coordinates.length(); ++i ) {
		start_indices[ i ] = n_coordinates[ i - 1 ] + start_indices[ i - 1 ] - 1;
	}

	//Rcpp::IntegerVector n_coordinates( total_coordinates, 3 );

	// Rcpp::Rcout << "n_coordinates: " << n_coordinates << std::endl;

	std::unordered_map< std::string, std::string > polygon_colours = mapdeck::layer_colours::fill_stroke_colours;
	Rcpp::StringVector polygon_legend = mapdeck::layer_colours::fill_stroke_legend;
	Rcpp::StringVector parameter_exclusions = Rcpp::StringVector::create("legend","legend_options","palette","na_colour");


	Rcpp::List interleaved = Rcpp::List::create(
		Rcpp::_["data"] = data,
		Rcpp::_["n_coordinates"] = n_coordinates,
		Rcpp::_["total_coordinates"] = total_coordinates
	);

	Rcpp::StringVector js_tri = jsonify::api::to_json(
		tri, false, digits,
		true, true, "column" // numeric_dates, factors_as_string, by -- not used on interleaved data
	);

	//return interleaved;

	Rcpp::List processed = spatialwidget::api::create_interleaved(
		interleaved,
		params,
		lst_defaults,
		polygon_colours,
		polygon_legend,
		total_coordinates,
		parameter_exclusions,
		true, // jsonify legend
		digits,
		"interleaved"
	);

	//return processed;

	//Rcpp::stop("stopping");

	Rcpp::List res = Rcpp::List::create(
		Rcpp::_["interleaved"] = js_tri,
		Rcpp::_["data"] = processed,
		Rcpp::_["start_indices"] = start_indices
	);

	return res;

	// Rcpp::List formatted_data = spatialwidget::api::format_data(
	// 		data,
	// 		params,
	// 		lst_defaults,
	// 		polygon_colours,
	// 		polygon_legend,
	// 		data_rows,
	// 		parameter_exclusions,
	// 		digits,
	// 		"interleaved"
	// );
	//
	// return formatted_data;

}

// // [[Rcpp::export]]
// Rcpp::List rcpp_polygon_quadmesh( Rcpp::DataFrame data,
//                                   Rcpp::List params,
//                                   Rcpp::List geometry_columns) {
//
// 	int data_rows = data.nrow();
//
// 	Rcpp::List lst_defaults = polygon_defaults( data_rows );  // initialise with defaults
// 	std::unordered_map< std::string, std::string > polygon_colours = mapdeck::polygon::polygon_colours;
// 	Rcpp::StringVector polygon_legend = mapdeck::polygon::polygon_legend;
// 	Rcpp::StringVector parameter_exclusions = Rcpp::StringVector::create("legend","legend_options","palette","na_colour");
//
//
// 	return spatialwidget::api::create_geojson_quadmesh(
// 		data,
// 		params,
// 		lst_defaults,
// 		polygon_colours,
// 		polygon_legend,
// 		geometry_columns,
// 		data_rows,
// 		parameter_exclusions,
// 		true  // jsonify legend
// 		);
// }


// [[Rcpp::export]]
Rcpp::List rcpp_polygon_polyline(
		Rcpp::DataFrame data,
		Rcpp::List params,
		Rcpp::StringVector geometry_columns
	) {

	int data_rows = data.nrows();

	Rcpp::List lst_defaults = polygon_defaults( data_rows );  // initialise with defaults

	std::unordered_map< std::string, std::string > polygon_colours = mapdeck::layer_colours::fill_stroke_colours;
	Rcpp::StringVector polygon_legend = mapdeck::layer_colours::fill_stroke_legend;
	Rcpp::StringVector parameter_exclusions = Rcpp::StringVector::create("legend","legend_options","palette","na_colour");

	return spatialwidget::api::create_polyline(
		data,
		params,
		lst_defaults,
		polygon_colours,
		polygon_legend,
		data_rows,
		parameter_exclusions,
		geometry_columns,
		true  // jsonify legend
	);
}

