#pragma once
#include "Globals.h"
#include "./structs/Transform.h"

class Mesh;
class Material;

struct MvpCB
{
	Matrix mvp;
};

struct ModelMatrixCB
{
	Matrix modelMatrix;
};

struct NormalMatrixCB
{
	Matrix normalMatrix;
};

class GameObject {
public:
	GameObject();
	~GameObject();
	Mesh& getMesh() { return *mesh; }
	const Mesh& getMesh() const { return *mesh; }
	void setMesh(Mesh& _mesh) { mesh = &_mesh; }
	const Material& getMaterial() const { return *material; }
	void setMaterial(Material& _material) { material = &_material; }
	Transform& getTransform() { return transform; }
	const Transform& getTransform() const { return transform; }
	void setTransform(Transform& _transform) { transform = _transform; }
	Matrix& getModelMatrix() { return model; }
	const Matrix& getModelMatrix() const { return model; }
	void setModelMatrix(Matrix _model) { model = _model; }
	Matrix& getMvpMatrix() { return mvp; }
	const Matrix& getMvpMatrix() const { return mvp; }
	void setMvpMatrix(Matrix _mvp) { mvp = _mvp; }

	ComPtr<ID3D12Resource> getMvpCB() { return mvpCB; }
	void setMvpCB(ComPtr<ID3D12Resource> _mvpCB) { mvpCB = _mvpCB; }

	ComPtr<ID3D12Resource> getModelCB() { return modelCB; }
	void setModelCB(ComPtr<ID3D12Resource> _modelCB) { modelCB = modelCB; }

	ComPtr<ID3D12Resource> getNormalCB() { return normalCB; }
	void setNormalCB(ComPtr<ID3D12Resource> normalCB) { normalCB = normalCB; }

	MvpCB* getMvpData() const { return mvpData; }
	void setMvpData(MvpCB* _mvpData) { mvpData = _mvpData; }

	ModelMatrixCB* getModelMatrixData() const { return modelData; }
	void setModelMatrixData(ModelMatrixCB* _modelMatrixData) { modelData = _modelMatrixData; }

	NormalMatrixCB* getNormalData() const { return normalData; }
	void setMvpData(NormalMatrixCB* _normalData) { normalData = _normalData; }

private:
	Mesh* mesh = nullptr;
	Material* material = nullptr;
	Transform transform = {};
	Matrix model;
	Matrix mvp;

	ComPtr<ID3D12Resource> mvpCB = nullptr;
	MvpCB* mvpData = nullptr;

	ComPtr<ID3D12Resource> modelCB = nullptr;
	ModelMatrixCB* modelData = nullptr;

	ComPtr<ID3D12Resource> normalCB = nullptr;
	NormalMatrixCB* normalData = nullptr;
};