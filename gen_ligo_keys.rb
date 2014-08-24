#!/usr/bin/env ruby

#
# This script reads a channel_bundling.csv and spits out all of the unique category
# values, to make it easier to create Bundle Key files. It could use some work.
#


require 'fileutils'
require 'csv'
require 'pp'

filePath = ARGV[0]

if !File.file?( filePath )
	puts "ERROR: File doesn't exist.\n#{filePath}"
	exit()
end

puts "Parsing #{filePath}"

column_names = []
column_values = []
row_num = 0
CSV.foreach( filePath ) do |row|
	if row_num == 0
  		# These are the column names 
  		puts "COLUMNS:\n"
  		#puts row.inspect 	 
  		row.each do |column|
  			col_name = column.strip().gsub(/#*\s*/,'')
  			column_names << col_name
  			#print "#{col_name}, "
			column_values << []
  		end
  		#print "\n"
  		puts column_names.inspect
	elsif ( row.length != column_names.length)
		puts "ODD ROW LENGTH:\n#{row.inspect}"
	else
		row.each_with_index do |val, index|
			if index > 0 # Ignore the channel names
				vals = column_values[index]
				vals << val
				column_values[index] = vals.uniq
			end
		end
	end
  row_num += 1
end

puts "Found #{row_num} columns"

column_values.each_with_index do |values, column_idx|
	puts ["-----", column_idx, "-----", values.map{|cv| "#{cv},#{cv}"} ]
end
#pp column_values
