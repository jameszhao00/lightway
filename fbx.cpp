/*
#include "fbx.h"

#include <algorithm>
#define FBXSDK_NEW_API
#include <fbxsdk.h>

namespace fbx
{		
	vec3 to_glm(FbxDouble3 d3)
	{
		return vec3(d3.mData[0], d3.mData[1], d3.mData[2]);
	}
	vec2 to_glm(FbxVector2 v)
	{
		return vec2(v.mData[0], v.mData[1]);
	}
	struct Mesh
	{
		Mesh() : num_tris(0) { }
		Triangle triangles[1024];
		int num_tris;
		vector<Material> materials;
	};
	unique_ptr<Mesh> mesh_to_asset_mesh(const FbxMesh * fbxmesh, const FbxAMatrix & world_t)
	{
		assert(fbxmesh->IsTriangleMesh());

		auto world_r = world_t;
		world_r.SetT(FbxVector4(0, 0, 0, 0));
		//world_r = world_r.Inverse().Transpose();
		unique_ptr<Mesh> model(new Mesh());

		auto normals_element = fbxmesh->GetLayer(0)->GetNormals();
		auto uvs_element = fbxmesh->GetLayer(0)->GetUVs();

		assert(normals_element->GetMappingMode() == FbxLayerElement::eByControlPoint);

		int tri_count = fbxmesh->GetPolygonCount();
		model->num_tris = tri_count;
				cout << " tri_count = " << tri_count << endl;

		for(int poly_i = 0; poly_i < tri_count; poly_i++)
		{
			Triangle tri;
			for(int vert_i = 0; vert_i < 3; vert_i++)
			{
				int cp = fbxmesh->GetPolygonVertex(poly_i, vert_i);
				tri.vertices[vert_i].position = to_glm(world_t.MultT(fbxmesh->GetControlPointAt(cp)));
				
				cout << tri.vertices[vert_i].position << endl;
				if(normals_element)
				{						
					FbxVector4 normal;
					if(normals_element->GetReferenceMode() == FbxLayerElement::eDirect)
					{
						normal = normals_element->GetDirectArray().GetAt(vert_i);
					}
					else
					{					
						auto index = normals_element->GetIndexArray().GetAt(vert_i);
						normal = normals_element->GetDirectArray().GetAt(index);
					}
					normal[3] = 0;
					auto normal_w = world_r.MultS(normal);
					
					
					normal_w.Normalize();
					tri.vertices[vert_i].normal = to_glm(normal_w);
					if(vert_i == 0) cout << "n pret" << to_glm(normal) << "t = " << to_glm(normal_w) << endl;
				}
				if(uvs_element)
				{
					FbxVector2 uv;
					if(uvs_element->GetReferenceMode() == FbxLayerElement::eDirect)
					{					
						uv = uvs_element->GetDirectArray().GetAt(vert_i);
					}
					else
					{
						auto index = uvs_element->GetIndexArray().GetAt(vert_i);
						uv = uvs_element->GetDirectArray().GetAt(index);
					}					
					tri.vertices[vert_i].uv = to_glm(uv);
				}
			}
			tri.compute_normal();
			model->triangles[poly_i] = tri;
				cout << " 2 = " << model->triangles[poly_i].vertices[1].position << endl;
		}

		return model;
	}
	void find_nodes(FbxNode* node, FbxNodeAttribute::EType type, list<FbxNode*>* output)
	{
		auto attribute = node->GetNodeAttribute();
		if(attribute != nullptr)
		{
			if(attribute->GetAttributeType() == type)
			{
				output->push_back(node);
			}
		}
		for(int i = 0; i < node->GetChildCount(); i++) 
		{
			find_nodes(node->GetChild(i), type, output);
		}
	}

	unique_ptr<StaticScene> load_fbx_scene(string path)
	{	
		auto fbx_manager = FbxManager::Create();			
			
		auto fbx_scene = FbxScene::Create(fbx_manager, "");
		auto importer = FbxImporter::Create(fbx_manager, "");
		auto error = importer->Initialize(path.c_str());
		cout << "fbx error " << error << endl;
		auto result = importer->Import(fbx_scene);
		cout << "fbx result " << result << endl;
		importer->Destroy();

		auto current_axis = fbx_scene->GetGlobalSettings().GetAxisSystem();			
		auto current_unit = fbx_scene->GetGlobalSettings().GetSystemUnit();

		//FbxSystemUnit desired_unit(FbxSystemUnit::cm);
		FbxAxisSystem desired_axis(FbxAxisSystem::eOpenGL);
		
		if (current_unit != desired_unit)
		{
			desired_unit.ConvertScene(scene);
		}
		
		if (current_axis != desired_axis)
		{
			//desired_axis.ConvertScene(fbx_scene);
		}			
		
		FbxGeometryConverter geo_converter(fbx_manager);
		list<FbxNode*> mesh_nodes;
		find_nodes(fbx_scene->GetRootNode(), FbxNodeAttribute::eMesh, &mesh_nodes);
		cout << " starting! " << endl;
		for(auto it = mesh_nodes.begin(); it != mesh_nodes.end(); it++)
		{
			cout << "poly count " << (*it)->GetMesh()->GetPolygonCount() << endl;
		}
		cout << "end stage 1"<<endl;
		unique_ptr<StaticScene> scene(new StaticScene());
		for(auto it = mesh_nodes.begin(); it != mesh_nodes.end(); it++)
		{
			FbxNode* mesh_node = *it;
			
			assert(mesh_node != nullptr);
			cout << "inner loop poly count 1 " << mesh_node->GetMesh()->GetPolygonCount() << " " << mesh_node->GetMesh()->GetPolygonVertexCount() << endl;
			//assert(geo_converter.TriangulateInPlace(mesh_node));
			//FbxMesh* mesh = mesh_node->GetMesh();
			FbxMesh* mesh = geo_converter.TriangulateMesh(mesh_node->GetMesh());
			
			mesh->ComputeVertexNormals();
			cout << "inner loop poly count 2 " << mesh->GetPolygonCount() << " " << mesh->GetPolygonVertexCount()<< endl;
			
			assert(mesh->SplitPoints(FbxLayerElement::eNormal));
			auto node_world_transform = fbx_scene->GetEvaluator()->GetNodeGlobalTransform(mesh_node);
			//need 1 uv / vert
			//assert(mesh->SplitPoints(FbxLayerElement::eTextureDiffuse));
			assert(0); //not implemented
			
			auto m = mesh_to_asset_mesh(mesh, node_world_transform);
			for(int tri_i = 0; tri_i < m->num_tris; tri_i++)
			{
				scene->triangles[scene->num_tris + tri_i] = m->triangles[tri_i];
				//cout << "new vert : " << endl;
				for(int vert_i = 0; vert_i < 3; vert_i++)
				{
					//cout<<"vert : " << m->triangles[tri_i].vertices[vert_i].position << endl;
				}
			}
			scene->num_tris += m->num_tris;
			
		//cout << "num tris : " << scene->num_tris << endl;
		}
		fbx_manager->Destroy();
		return scene;
	}
}
*/