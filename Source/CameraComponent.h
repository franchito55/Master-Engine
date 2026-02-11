#include "Globals.h"
#include "GameObject.h"

class CameraComponent : public GameObject {
public:
	CameraComponent();

	void render(ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_VERTEX_BUFFER_VIEW& vBV, D3D12_INDEX_BUFFER_VIEW& iBV) override;
	void update() override;

	float getFov() { return fov; }
	const float getFov() const { return fov; }
	void setFov(float _fov) { fov = _fov; }

	float getNearPlane() { return nearPlane; }
	const float getNearPlane() const { return nearPlane; }
	void setNearPlane(float _nearPlane) { nearPlane = _nearPlane; }

	float getFarPlane() { return farPlane; }
	const float getFarPlane() const { return farPlane; }
	void setFarPlane(float _farPlane) { farPlane = _farPlane; }

	const Matrix& getViewMatrix() const { return view; }
	void setViewMatrix(Matrix& _view) { view = _view; }

	const Matrix& getProjectionMatrix() const { return projection; }
	void setProjectionMatrix(Matrix& _projection) { projection = _projection; }

	const Frustum& getFrustum() const { return frustum; }
	void setFrustum(Frustum& _frustum) { frustum = _frustum; }

	void recalculateFrustum();

private:
	float fov = 90.0f;
	float nearPlane = 1.0f;
	float farPlane = 10.0f;

	Matrix view = {};
	Matrix projection = {};

	Frustum frustum = {};

	Vector3* calculateFrustumVerticesFromFrustum(Vector3 verts[8]);
};