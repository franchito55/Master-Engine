#pragma once
#include "Globals.h"
#include "./structs/Transform.h"

class Mesh;
class Material;

struct Frustum {
	Plane frontFace;
	Plane backFace;
	Plane topFace;
	Plane bottomFace;
	Plane leftFace;
	Plane rightFace;
};

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

struct AABB {
	Vector3 points[8];

	bool isPointInsidePlane(const Vector3& point, const Plane& plane) {
		return plane.Normal().Dot(point) + plane.D() < 0;
	}

	bool isFullyOutsideOfPlane(const Plane& plane) {
		for (auto& point : points) {
			if (isPointInsidePlane(point, plane)) {
				return false;
			}
		}
		return true;
	}

	bool test(const Frustum& frustum) {
		return !isFullyOutsideOfPlane(frustum.frontFace)
			&& !isFullyOutsideOfPlane(frustum.backFace)
			&& !isFullyOutsideOfPlane(frustum.rightFace)
			&& !isFullyOutsideOfPlane(frustum.leftFace)
			&& !isFullyOutsideOfPlane(frustum.topFace)
			&& !isFullyOutsideOfPlane(frustum.bottomFace);
	}
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

	ComPtr<ID3D12Resource>& getMvpCB() { return mvpCB; }
	void setMvpCB(ComPtr<ID3D12Resource>& _mvpCB) { mvpCB = _mvpCB; }

	ComPtr<ID3D12Resource>& getModelCB() { return modelCB; }
	void setModelCB(ComPtr<ID3D12Resource>& _modelCB) { modelCB = modelCB; }

	ComPtr<ID3D12Resource>& getNormalCB() { return normalCB; }
	void setNormalCB(ComPtr<ID3D12Resource>& normalCB) { normalCB = normalCB; }

	MvpCB*& getMvpData() { return mvpData; }
	void setMvpData(MvpCB* _mvpData) { mvpData = _mvpData; }

	ModelMatrixCB*& getModelMatrixData() { return modelData; }
	void setModelMatrixData(ModelMatrixCB* _modelMatrixData) { modelData = _modelMatrixData; }

	NormalMatrixCB*& getNormalData() { return normalData; }
	void setNormalData(NormalMatrixCB* _normalData) { normalData = _normalData; }

	AABB getAABB() const { return aabb; }
	void setAABB(AABB _aabb) { aabb = _aabb; }


	virtual void render(ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_VERTEX_BUFFER_VIEW& vBV, D3D12_INDEX_BUFFER_VIEW& iBV);
	virtual void update() {};

private:
	Mesh* mesh = nullptr;
	Material* material = nullptr;
	Matrix mvp;

	ComPtr<ID3D12Resource> mvpCB = nullptr;
	MvpCB* mvpData = nullptr;

	ComPtr<ID3D12Resource> modelCB = nullptr;
	ModelMatrixCB* modelData = nullptr;

	ComPtr<ID3D12Resource> normalCB = nullptr;
	NormalMatrixCB* normalData = nullptr;

	AABB aabb = {};
	
protected:
	Transform transform = {};
	Matrix model;
};